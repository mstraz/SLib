#ifndef CHECKHEADER_SLIB_VIEW_PAGE
#define CHECKHEADER_SLIB_VIEW_PAGE

#include "definition.h"

#include "view.h"
#include "transition.h"

SLIB_UI_NAMESPACE_BEGIN

class ViewPage;
class ViewPager;

class SLIB_EXPORT IViewPagerListener
{
public:
	virtual void onPageAction(ViewPager* pager, View* page, UIPageAction action);
	
	virtual void onFinishPageAnimation(ViewPager* pager, View* page, UIPageAction action);
	
};

class SLIB_EXPORT ViewPager : public View
{
	SLIB_DECLARE_OBJECT
	
public:
	ViewPager();
	
public:
	sl_size getPagesCount();
	
protected:
	virtual void onPageAction(View* page, UIPageAction action);

	virtual void onFinishPageAnimation(View* page, UIPageAction action);
	
public:
	virtual void dispatchPageAction(View* page, UIPageAction action);
	
	virtual void dispatchFinishPageAnimation(View* page, UIPageAction action);

public:
	// override
	void onResize(sl_ui_len width, sl_ui_len height);

public:
	SLIB_PTR_PROPERTY(IViewPagerListener, Listener)
	
protected:
	CList< Ref<View> > m_pages;
	
};

class SLIB_EXPORT ViewStack : public ViewPager
{
	SLIB_DECLARE_OBJECT
	
public:
	ViewStack();
	
public:
	void push(const Ref<View>& page, const Transition& transition, sl_bool flagRemoveAllBackPages = sl_false);

	void push(const Ref<View>& page, sl_bool flagRemoveAllBackPages = sl_false);
	
	void pop(const Ref<View>& page, const Transition& transition);
	
	void pop(const Ref<View>& page);
	
	void pop(const Transition& transition);
	
	void pop();

	TransitionType getPushTransitionType();
	
	void setPushTransitionType(TransitionType type);
	
	TransitionDirection getPushTransitionDirection();
	
	void setPushTransitionDirection(TransitionDirection direction);
	
	TransitionType getPopTransitionType();
	
	void setPopTransitionType(TransitionType type);
	
	TransitionDirection getPopTransitionDirection();
	
	void setPopTransitionDirection(TransitionDirection direction);
	
	void setTransitionType(TransitionType type);
	
	void setTransitionDirection(TransitionDirection direction);

	float getTransitionDuration();
	
	void setTransitionDuration(float duration);
	
	AnimationCurve getTransitionCurve();
	
	void setTransitionCurve(AnimationCurve curve);
	
protected:
	void _push(Ref<View> page, Transition transition, sl_bool flagRemoveAllBackPages);
	
	void _pop(Ref<View> page, Transition transition);
	
	void _applyDefaultPushTransition(Transition& transition);
	
	void _applyDefaultPopTransition(Transition& transition);
	
protected:
	TransitionType m_pushTransitionType;
	TransitionDirection m_pushTransitionDirection;
	TransitionType m_popTransitionType;
	TransitionDirection m_popTransitionDirection;
	float m_transitionDuration; // seconds
	AnimationCurve m_transitionCurve; // curve
	
};

class SLIB_EXPORT ViewPage : public ViewGroup
{
	SLIB_DECLARE_OBJECT
	
public:
	ViewPage();
	
protected:
	virtual void onOpen();
	
	virtual void onClose();
	
	virtual void onResume();
	
	virtual void onPause();
	
	virtual void onPageAction(UIPageAction action);
	
	virtual void onFinishPageAnimation(UIPageAction action);
	
public:
	virtual void dispatchPageAction(ViewPager* pager, UIPageAction action);
	
	virtual void dispatchFinishPageAnimation(ViewPager* pager, UIPageAction action);
	
};


SLIB_UI_NAMESPACE_END

#endif