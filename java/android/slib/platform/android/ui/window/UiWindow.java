package slib.platform.android.ui.window;

import slib.platform.android.Logger;
import slib.platform.android.SlibActivity;
import slib.platform.android.ui.UiThread;
import slib.platform.android.ui.view.UiGroupView;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.graphics.PointF;
import android.graphics.RectF;
import android.os.Build;
import android.os.Build.VERSION;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.FrameLayout;

public class UiWindow extends UiGroupView {

	Activity activity;
	public long instance;
	int backgroundColor;
	
	private UiWindow(Context context) {
		super(context);
		instance = 0;
		backgroundColor = 0;
		if (context instanceof SlibActivity) {
			((SlibActivity)context).onCreateWindow(this);
		}
	}
	
	static UiWindow create(Activity activity
			, boolean flagFullScreen, boolean flagCenterScreen
			, float x, float y, float width, float height) {
		try {
			UiWindow ret = new UiWindow(activity);
			FrameLayout.LayoutParams params;
			if (flagFullScreen) {
				params = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT);			
			} else {
				params = new FrameLayout.LayoutParams((int)(width), (int)(height));
				if (flagCenterScreen) {
					params.gravity = Gravity.CENTER;
				} else {
					params.leftMargin = (int)x;
					params.topMargin = (int)y;
				}
			}
			activity.addContentView(ret, params);
			ret.activity = activity;
			return ret;			
		} catch (Exception e) {
			Logger.exception(e);
			return null;
		}
	}
	
	public View getContentView() {
		return this;
	}
	
	public void close() {
		try {
			ViewParent parent = getParent();
			if (parent != null && parent instanceof ViewGroup) {
				((ViewGroup)parent).removeView(this);
			}
			if (activity instanceof SlibActivity) {
				((SlibActivity)activity).onCloseWindow(this);
			}			
		} catch (Exception e) {
			Logger.exception(e);
		}
	}
	
	int getWindowBackgroundColor() {
		return backgroundColor;
	}
	
	public void setWindowBackgroundColor(int color) {
		backgroundColor = color;
		setBackgroundColor(color);
	}
	
	public RectF getFrame() {
		RectF ret = new RectF();
		int[] off = new int[2];
		getLocationOnScreen(off);
		ret.left = off[0];
		ret.top = off[1];
		ret.right = ret.left + getWidth();
		ret.bottom = ret.top + getHeight();
		return ret;
	}
	
	public void setFrame(float left, float top, float right, float bottom) {
		try {
			final FrameLayout.LayoutParams params = new FrameLayout.LayoutParams((int)(right - left), (int)(bottom - top));
			params.leftMargin = (int)left;
			params.topMargin = (int)top;
			if (UiThread.isUiThread()) {
				setLayoutParams(params);
			} else {
				activity.runOnUiThread(new Runnable() {
					public void run() {
						setLayoutParams(params);
					}
				});
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}
	
	public PointF getSize() {
		PointF ret = new PointF();
		ret.x = getWidth();
		ret.y = getHeight();
		return ret;
	}
	
	public void setSize(float width, float height) {
		try {
			final FrameLayout.LayoutParams params = (FrameLayout.LayoutParams)(getLayoutParams());
			if (params != null) {
				params.width = (int)width;
				params.height = (int)height;
			}
			if (UiThread.isUiThread()) {
				setLayoutParams(params);
			} else {
				activity.runOnUiThread(new Runnable() {
					public void run() {
						setLayoutParams(params);
					}
				});
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}
	
	public boolean isVisible() {
		return getVisibility() != View.VISIBLE;
	}
	
	public void setVisible(boolean flag) {
		setVisibility(flag ? View.VISIBLE : View.INVISIBLE);
	}

	public boolean isAlwaysOnTop() {
		return false;
	}
	
	public void setAlwaysOnTop(boolean flag) {
	}
	
	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	float _getAlpha() {
		return getAlpha();
	}
	public float getWindowAlpha() {
		if (VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			return _getAlpha();			
		} else {
			return 1;
		}
	}
	
	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	void _setAlpha(float alpha) {
		setAlpha(alpha);
	}
	public void setWindowAlpha(float alpha) {
		if (alpha < 0) {
			alpha = 0;
		}
		if (alpha > 1) {
			alpha = 1;
		}
		if (VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			_setAlpha(alpha);
		}
	}
	
	public void focus() {
		requestFocus();
	}
	
	public PointF convertCoordinateFromScreenToWindow(float x, float y) {
		PointF ret = new PointF();
		int[] location = new int[2];
		getLocationOnScreen(location);
		ret.x = x - location[0];
		ret.y = y - location[1];
		return ret;
	}
	
	public PointF convertCoordinateFromWindowToScreen(float x, float y) {
		PointF ret = new PointF();
		int[] location = new int[2];
		getLocationOnScreen(location);
		ret.x = location[0] + x;
		ret.y = location[1] + y;
		return ret;
	}
	
	private static native void nativeOnResize(long instance, float w, float h);
	public void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
		nativeOnResize(instance, w, h);
    }
	
	private static native boolean nativeOnClose(long instance);
	public void onClose() {
		if (nativeOnClose(instance)) {
			close();
		}
	}
}
