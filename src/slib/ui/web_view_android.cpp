/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_ANDROID)

#include "slib/ui/web_view.h"

#include "view_android.h"

namespace slib
{

	class _priv_WebView : public WebView
	{
	public:
		void _load(jobject handle);

		static void JNICALL nativeOnStartLoad(JNIEnv* env, jobject _this, jlong instance, jstring jurl)
		{
			Ref<View> _view = Android_ViewInstance::findView(instance);
			if (_priv_WebView* view = CastInstance<_priv_WebView>(_view.get())) {
				String url = Jni::getString(jurl);
				view->dispatchStartLoad(url);
			}
		}

		static void JNICALL nativeOnFinishLoad(JNIEnv* env, jobject _this, jlong instance, jstring jurl)
		{
			Ref<View> _view = Android_ViewInstance::findView(instance);
			if (_priv_WebView* view = CastInstance<_priv_WebView>(_view.get())) {
				String url = Jni::getString(jurl);
				view->dispatchFinishLoad(url, sl_false);
			}
		}

		static void JNICALL nativeOnErrorLoad(JNIEnv* env, jobject _this, jlong instance, jstring jurl, jstring jerror)
		{
			Ref<View> _view = Android_ViewInstance::findView(instance);
			if (_priv_WebView* view = CastInstance<_priv_WebView>(_view.get())) {
				view->m_lastErrorMessage = Jni::getString(jerror);
				String url = Jni::getString(jurl);
				view->dispatchFinishLoad(url, sl_true);
			}
		}

		static void JNICALL nativeOnMessage(JNIEnv* env, jobject _this, jlong instance, jstring jmsg, jstring jparam)
		{
			Ref<View> _view = Android_ViewInstance::findView(instance);
			if (_priv_WebView* view = CastInstance<_priv_WebView>(_view.get())) {
				String msg = Jni::getString(jmsg);
				if (msg.isNotEmpty()) {
					String param = Jni::getString(jparam);
					view->dispatchMessageFromJavaScript(msg, param);
				}
			}
		}
	};

	SLIB_JNI_BEGIN_CLASS(JAndroidWebView, "slib/platform/android/ui/view/UiWebView")

		SLIB_JNI_STATIC_METHOD(create, "_create", "(Landroid/content/Context;)Lslib/platform/android/ui/view/UiWebView;");

		SLIB_JNI_STATIC_METHOD(load, "_load", "(Landroid/view/View;Ljava/lang/String;)V");
		SLIB_JNI_STATIC_METHOD(loadHTML, "_loadHTML", "(Landroid/view/View;Ljava/lang/String;Ljava/lang/String;)V");
		SLIB_JNI_STATIC_METHOD(getURL, "_getURL", "(Landroid/view/View;)Ljava/lang/String;");
		SLIB_JNI_STATIC_METHOD(getTitle, "_getTitle", "(Landroid/view/View;)Ljava/lang/String;");
		SLIB_JNI_STATIC_METHOD(goBack, "_goBack", "(Landroid/view/View;)V");
		SLIB_JNI_STATIC_METHOD(goForward, "_goForward", "(Landroid/view/View;)V");
		SLIB_JNI_STATIC_METHOD(reload, "_reload", "(Landroid/view/View;)V");
		SLIB_JNI_STATIC_METHOD(runJavaScript, "_runJavaScript", "(Landroid/view/View;Ljava/lang/String;)V");
		SLIB_JNI_STATIC_METHOD(clearCache, "_clearCache", "(Landroid/view/View;)V");
		SLIB_JNI_STATIC_METHOD(setCustomUserAgent, "_setCustomUserAgent", "(Landroid/view/View;Ljava/lang/String;)V");

		SLIB_JNI_NATIVE(nativeOnStartLoad, "nativeOnStartLoad", "(JLjava/lang/String;)V", _priv_WebView::nativeOnStartLoad);
		SLIB_JNI_NATIVE(nativeOnFinishLoad, "nativeOnFinishLoad", "(JLjava/lang/String;)V", _priv_WebView::nativeOnFinishLoad);
		SLIB_JNI_NATIVE(nativeOnErrorLoad, "nativeOnErrorLoad", "(JLjava/lang/String;Ljava/lang/String;)V", _priv_WebView::nativeOnErrorLoad);
		SLIB_JNI_NATIVE(nativeOnMessage, "nativeOnMessage", "(JLjava/lang/String;Ljava/lang/String;)V", _priv_WebView::nativeOnMessage);

	SLIB_JNI_END_CLASS

	void _priv_WebView::_load(jobject handle)
	{
		JniLocal<jstring> jurl = Jni::getJniString(m_urlOrigin);
		if (m_flagOfflineContent) {
			JniLocal<jstring> jcontent = Jni::getJniString(m_offlineContentHTML);
			JAndroidWebView::loadHTML.call(sl_null, handle, jcontent.get(), jurl.get());
		} else {
			JAndroidWebView::load.call(sl_null, handle, jurl.get());
		}
	}

	Ref<ViewInstance> WebView::createNativeWidget(ViewInstance* _parent)
	{
		Ref<Android_ViewInstance> ret;
		Android_ViewInstance* parent = (Android_ViewInstance*)_parent;
		if (parent) {
			JniLocal<jobject> handle = JAndroidWebView::create.callObject(sl_null, parent->getContext());
			ret = Android_ViewInstance::create<Android_ViewInstance>(this, parent, handle.get());
			if (ret.isNotNull()) {
				jobject handle = ret->getHandle();
				String customUserAgent = m_customUserAgent;
				if (customUserAgent.isNotEmpty()) {
					JniLocal<jstring> jvalue = Jni::getJniString(customUserAgent);
					JAndroidWebView::setCustomUserAgent.call(sl_null, handle, jvalue.get());
				}
				((_priv_WebView*)this)->_load(handle);
			}
		}
		return ret;
	}

	void WebView::_refreshSize_NW()
	{
	}

	void WebView::_load_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			((_priv_WebView*)this)->_load(handle);
		}
	}

	String WebView::_getURL_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			return JAndroidWebView::getURL.callString(sl_null, handle);
		}
		return sl_null;
	}

	String WebView::_getPageTitle_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			return JAndroidWebView::getTitle.callString(sl_null, handle);
		}
		return sl_null;
	}

	void WebView::_goBack_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			JAndroidWebView::goBack.call(sl_null, handle);
		}
	}

	void WebView::_goForward_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			JAndroidWebView::goForward.call(sl_null, handle);
		}
	}

	void WebView::_reload_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			JAndroidWebView::reload.call(sl_null, handle);
		}
	}

	void WebView::_clearCache_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			JAndroidWebView::clearCache.call(sl_null, handle);
		}
	}

	void WebView::_runJavaScript_NW(const String& script)
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			JniLocal<jstring> jscript = Jni::getJniString(script);
			JAndroidWebView::runJavaScript.call(sl_null, handle, jscript.get());
		}
	}

	void WebView::_setCustomUserAgent_NW()
	{
		jobject handle = UIPlatform::getViewHandle(this);
		if (handle) {
			JniLocal<jstring> jvalue = Jni::getJniString(m_customUserAgent);
			JAndroidWebView::setCustomUserAgent.call(sl_null, handle, jvalue.get());
		}
	}

}

#endif
