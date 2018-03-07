/*
 *  Copyright (c) 2008-2017 SLIBIO. All Rights Reserved.
 *
 *  This file is part of the SLib.io project.
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_IOS)

#include "slib/ui/core.h"
#include "slib/ui/render_view.h"
#include "slib/render/opengl.h"
#include "slib/ui/mobile_app.h"

#include "view_ios.h"

#include <GLKit/GLKit.h>

@interface SLib_iOS_GLView : GLKView {
	
	@public slib::WeakRef<slib::iOS_ViewInstance> m_viewInstance;
	
	@public sl_bool m_flagRenderingContinuously;
	@public sl_bool m_flagRequestRender;
	
	id m_renderer;
	slib::Ref<slib::RenderEngine> m_engine;
	CADisplayLink* m_displayLink;
	
	CGFloat m_viewportWidth;
	CGFloat m_viewportHeight;
}

-(void)_init;
-(void)_setRenderContinuously:(BOOL)flag;
-(void)_requestRender;

@end


namespace slib
{
	Ref<ViewInstance> RenderView::createNativeWidget(ViewInstance* _parent)
	{
		SLib_iOS_GLView* handle = nil;
		IOS_VIEW_CREATE_INSTANCE_BEGIN
		handle = [[SLib_iOS_GLView alloc] initWithFrame:frame];
		IOS_VIEW_CREATE_INSTANCE_END
		if (handle != nil && ret.isNotNull()) {
			[handle _init];
			[handle _setRenderContinuously:(m_redrawMode == RedrawMode::Continuously)];
		}
		return ret;
	}
	
	void RenderView::_setRedrawMode_NW(RedrawMode mode)
	{
		UIView* handle = UIPlatform::getViewHandle(this);
		if (handle != nil && [handle isKindOfClass:[SLib_iOS_GLView class]]) {
			SLib_iOS_GLView* v = (SLib_iOS_GLView*)handle;
			[v _setRenderContinuously:(mode == RedrawMode::Continuously)];
		}
	}
	
	void RenderView::_requestRender_NW()
	{
		UIView* view = UIPlatform::getViewHandle(this);
		if (view != nil && [view isKindOfClass:[SLib_iOS_GLView class]]) {
			SLib_iOS_GLView* v = (SLib_iOS_GLView*)view;
			[v _requestRender];
		}
	}	
}

@interface SLib_iOS_GLViewRenderer : NSObject
{
	@public __weak SLib_iOS_GLView* m_view;
	@public __weak CADisplayLink* m_displayLink;
	
	BOOL m_flagRunning;
	BOOL m_flagViewVisible;
}
@end

@implementation SLib_iOS_GLViewRenderer

- (void)onGLRenderFrame
{
	if (!m_flagRunning) {
		return;
	}
	SLib_iOS_GLView* view = m_view;
	if (view != nil) {
		if (m_flagViewVisible && !(slib::MobileApp::isPaused())) {
			if (view->m_flagRenderingContinuously || view->m_flagRequestRender) {
				view->m_flagRequestRender = sl_false;
				[view display];
			}
		}
	}
}

- (void)queryViewState
{
	SLib_iOS_GLView* view = m_view;
	if (view == nil) {
		return;
	}
	BOOL flagVisible = NO;
	sl_uint32 width = (sl_uint32)(view.frame.size.width * view.contentScaleFactor);
	sl_uint32 height = (sl_uint32)(view.frame.size.height * view.contentScaleFactor);
	view->m_viewportWidth = width;
	view->m_viewportHeight = height;
	if (width > 0 && height > 0) {
		UIView* viewCheck = view;
		for (;;) {
			if (viewCheck.hidden) {
				break;
			} else {
				UIView* parent = viewCheck.superview;
				if (parent == nil) {
					flagVisible = YES;
					break;
				}
				viewCheck = parent;
			}
		}
	}
	m_flagViewVisible = flagVisible;
}

- (void)onRun
{
	m_flagRunning = YES;
	m_flagViewVisible = NO;
#if defined(SLIB_PLATFORM_IS_IOS_SIMULATOR)
	slib::TimeCounter timer;
	int frameNumber = 0;
	while (m_flagRunning) {
		if (frameNumber % 10 == 0) {
			SLib_iOS_GLView* view = m_view;
			if (view != nil) {
				dispatch_async(dispatch_get_main_queue(), ^{
					[self queryViewState];
				});
			}
		}
		[self onGLRenderFrame];
		sl_uint64 t = timer.getElapsedMilliseconds();
		if (t < 15) {
			[NSThread sleepForTimeInterval:(15 - (sl_uint32)(t)) / 1000.0];
		}
		timer.reset();
		frameNumber++;
	}
#else
	NSRunLoop* loop = [NSRunLoop currentRunLoop];
	[m_displayLink addToRunLoop:loop forMode:NSDefaultRunLoopMode];
	[loop run];
#endif
}

- (void)stop
{
	m_flagRunning = NO;
}

@end

@implementation SLib_iOS_GLView

-(void)_init
{
	m_flagRenderingContinuously = sl_false;
	m_flagRequestRender = sl_true;
	m_viewportWidth = 0;
	m_viewportHeight = 0;
	
	EAGLContext* context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
	if (context == nil) {
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	}
	if (context != nil) {
		self.context = context;
	}
	
	self.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
	self.drawableDepthFormat = GLKViewDrawableDepthFormat24;
	
	SLib_iOS_GLViewRenderer* renderer = [[SLib_iOS_GLViewRenderer alloc] init];
	renderer->m_view = self;
	m_displayLink = [CADisplayLink displayLinkWithTarget:renderer selector:@selector(onGLRenderFrame)];
	renderer->m_displayLink = m_displayLink;
	[NSThread detachNewThreadSelector:@selector(onRun) toTarget:renderer withObject:nil];
	m_renderer = renderer;
	
}

-(void)dealloc
{
	[m_displayLink invalidate];
	[m_renderer stop];
}

-(void)_requestRender
{
	m_flagRequestRender = sl_true;
}

-(void)_setRenderContinuously:(BOOL)flag
{
	m_flagRenderingContinuously = flag;
}

- (void)drawRect:(CGRect)dirtyRect
{
	slib::Ref<slib::iOS_ViewInstance> instance = m_viewInstance;
	if (instance.isNull()) {
		return;
	}
	
	slib::Ref<slib::View> _view = instance->getView();
	if (slib::RenderView* view = slib::CastInstance<slib::RenderView>(_view.get())) {
		if (m_engine.isNull()) {
			m_engine = slib::GLES::createEngine();
		}
		if (m_engine.isNotNull()) {
			m_engine->setViewport(0, 0, m_viewportWidth, m_viewportHeight);
			view->dispatchFrame(m_engine.get());
		}
	}

}

- (void)setNeedsDisplay
{
}

- (void)setFrame:(CGRect)frame
{
	[super setFrame:frame];
}

IOS_VIEW_EVENTS

@end

#endif
