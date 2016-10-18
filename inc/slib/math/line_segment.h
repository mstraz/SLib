#ifndef CHECKHEADER_SLIB_MATH_LINE_SEGMENT
#define CHECKHEADER_SLIB_MATH_LINE_SEGMENT

#include "definition.h"

#include "point.h"
#include "matrix3.h"

SLIB_MATH_NAMESPACE_BEGIN

template <class T>
class SLIB_EXPORT LineSegmentT
{
public:
	PointT<T> point1;
	PointT<T> point2;
	
public:
	LineSegmentT() = default;
	
	LineSegmentT(const LineSegmentT<T>& other) = default;
	
	template <class O>
	LineSegmentT(const LineSegmentT<O>& other);
	
	LineSegmentT(const PointT<T>& point1, const PointT<T>& point2);
	
	LineSegmentT(T x1, T y1, T x2, T y2);
	
public:
	Vector2T<T> getDirection() const;
	
	T getLength2p() const;
	
	T getLength() const;
	
	void transform(const Matrix3T<T>& mat);
	
	PointT<T> projectPoint(const PointT<T>& point) const;
	
	T getDistanceFromPoint(const PointT<T>& point) const;
	
	T getDistanceFromPointOnInfiniteLine(const PointT<T>& point) const;
	
public:
	LineSegmentT<T>& operator=(const LineSegmentT<T>& other) = default;
	
	template <class O>
	LineSegmentT<T>& operator=(const LineSegmentT<O>& other);
	
};

SLIB_DECLARE_GEOMETRY_TYPE(LineSegment)

SLIB_MATH_NAMESPACE_END


SLIB_MATH_NAMESPACE_BEGIN

template <class T>
template <class O>
SLIB_INLINE LineSegmentT<T>::LineSegmentT(const LineSegmentT<O>& other)
: point1(other.point1), point2(other.point2)
{
}

template <class T>
template <class O>
SLIB_INLINE LineSegmentT<T>& LineSegmentT<T>::operator=(const LineSegmentT<O>& other)
{
	point1 = other.point1;
	point2 = other.point2;
	return *this;
}

SLIB_MATH_NAMESPACE_END

#endif