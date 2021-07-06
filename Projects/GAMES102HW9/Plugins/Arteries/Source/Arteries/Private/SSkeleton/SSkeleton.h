// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include <cmath>
#include <vector>
#include <tuple>
#include <list>
#include <queue>
#include <map>
#include <algorithm>

template<typename T>
struct TypeParseTraits;

#define DECLARE_PARSE_TYPE(X) template <> struct TypeParseTraits<X> { static const char* name; }
#define IMPLEMENT_PARSE_TYPE(X) const char* TypeParseTraits<X>::name = #X

#define GCNEW(x) (((GCollector&)GC).CNew(new x))
class CollectorBase
{
public:
	virtual ~CollectorBase() {}
	virtual void Collect() = 0;
};
template<typename T>
class Collector : public CollectorBase
{
public:
	std::list<T*> ptrs;
	T* cnew(T* t)
	{
		ptrs.push_back(t);
		return t;
	}
	virtual void Collect()
	{
		while (!ptrs.empty())
		{
			delete ptrs.front();
			ptrs.pop_front();
		}
	}
};
class GCollector
{
public:
	std::map<const char*, CollectorBase*> collectors;
	template<typename T>
	T* CNew(T* t)
	{
		const char* key = TypeParseTraits<T>::name;
		std::map<const char*, CollectorBase*>::iterator it = collectors.find(key);
		Collector<T>* collector;
		if (it == collectors.end())
		{
			collector = new Collector<T>;
			collectors[key] = collector;
		}
		else
			collector = (Collector<T>*)it->second;
		return collector->cnew(t);
	}
	void Collect()
	{
		for (std::map<const char*, CollectorBase*>::iterator it = collectors.begin(); it != collectors.end(); it++)
		{
			it->second->Collect();
			delete it->second;
		}
	}
};

#define MAX_dbl	(1.7976931348623158e+308)
#define EPS_dbl	(1e-12)

namespace std
{
	template<typename OptionalType>
	struct optional
	{
	public:
		optional(const OptionalType& InValue) :Value(InValue), bIsSet(true) {}
		optional() : bIsSet(false) {}
		optional(const optional& InValue): bIsSet(false)
		{
			if (InValue.bIsSet)
			{
				Value = InValue.Value;
				bIsSet = true;
			}
		}
		optional& operator=(const optional& InValue)
		{
			if (InValue.bIsSet)
			{
				Value = InValue.Value;
				bIsSet = true;
			}
			return *this;
		}
		optional& operator=(const OptionalType& InValue)
		{
			Value = InValue;
			bIsSet = true;
			return *this;
		}
		friend bool operator==(const optional& lhs, const optional& rhs)
		{
			if (lhs.bIsSet != rhs.bIsSet)
				return false;
			if (!lhs.bIsSet) // both unset
				return true;
			return lhs.Value == rhs.Value;
		}
		friend bool operator!=(const optional& lhs, const optional& rhs)
		{
			return !(lhs == rhs);
		}
		bool isSet() const { return bIsSet; }
		explicit operator bool() const { return bIsSet; }
		operator OptionalType() const { return Value; }
		explicit operator OptionalType const&() const { return Value; }
	//	operator const OptionalType&() const { return Value; }
		const OptionalType* operator->() const { return &Value; }
	private:
		OptionalType Value;
		bool bIsSet;
	};
};
template<class T>
T const& validate(std::optional<T> const& o)
{
	if (!o)
		throw std::overflow_error("Arithmetic overflow");
	return (T const&)o;
}
class Rational
{
public:
	Rational() : mN(0), mD(1) {}
	Rational(double aN) : mN(aN), mD(1) {}
	Rational(double aN, double aD) : mN(aN), mD(aD) {}
	double n() const { return mN; }
	double d() const { return mD; }
	double to_nt() const { return mN / mD; }
private:
	double mN, mD;
};
template<typename T>
class Uncertain
{
	T _i, _s;
public:
	Uncertain() : _i(), _s() {}
	Uncertain(T t) : _i(t), _s(t) {}
	Uncertain(T i, T s) : _i(i), _s(s)
	{
	}
	Uncertain& operator=(T t)
	{
		_i = _s = t;
		return *this;
	}
	T inf() const { return _i; }
	T sup() const { return _s; }
	bool is_same(Uncertain u) const { return _i == u._i && _s == u._s; }
	bool is_certain() const { return _i == _s; }
	T make_certain() const
	{
		if (is_certain())
			return _i;
		throw 0;
	}
	operator T() const
	{
		return make_certain();
	}
	static Uncertain indeterminate();
};
enum Sign
{
	NEGATIVE = -1, ZERO = 0, POSITIVE = 1,
	RIGHT_TURN = -1, LEFT_TURN = 1,
	CLOCKWISE = -1, COUNTERCLOCKWISE = 1,
	COLLINEAR = 0, COPLANAR = 0, DEGENERATE = 0,
	ON_NEGATIVE_SIDE = -1, ON_ORIENTED_BOUNDARY = 0, ON_POSITIVE_SIDE = 1,
	SMALLER = -1, EQUAL = 0, LARGER = 1
};
template<>
inline Uncertain<bool> Uncertain<bool>::indeterminate()
{
	return Uncertain<bool>(false,true);
}
template<>
inline Uncertain<Sign> Uncertain<Sign>::indeterminate()
{
	return Uncertain<Sign>(NEGATIVE, POSITIVE);
}
template < typename T >
inline bool is_certain(Uncertain<T> a)
{
	return a.is_certain();
}

template < typename T >
inline T get_certain(Uncertain<T> a)
{
	return a.inf();
}
inline bool certainly(Uncertain<bool> c)
{
	return is_certain(c) && get_certain(c);
}
inline Uncertain<bool> is_zero(const double& x)
{
	return Uncertain<bool>(abs(x) < EPS_dbl);
}
inline Uncertain<bool> certified_is_zero(const double& x)
{
	return std::isfinite(x) ? is_zero(x) : Uncertain<bool>::indeterminate();
}
inline Uncertain<bool> certified_is_not_zero(const double& x)
{
	return std::isfinite(x) ? Uncertain<bool>(!is_zero(x)) : Uncertain<bool>::indeterminate();
}
inline double const& validate(double const& n)
{
	if (!std::isfinite(n))
		throw std::overflow_error("Arithmetic overflow");
	return n;
}
inline Uncertain<Sign> certified_sign(const double& x)
{
	return std::isfinite(x) ? Uncertain<Sign>(x > EPS_dbl ? POSITIVE : (x < -EPS_dbl ? NEGATIVE : ZERO)) : Uncertain<Sign>::indeterminate();
}
inline Uncertain<bool> certified_is_positive(const Rational& x)
{
	Uncertain<Sign> signum = certified_sign(x.n());
	Uncertain<Sign> sigden = certified_sign(x.d());
	Uncertain<Sign> zero(ZERO);
	return (signum != zero) & (signum == sigden);
}
inline Uncertain<Sign> certified_compare(const double& n1, const double& n2)
{
	return std::isfinite(n1) && std::isfinite(n2) ? Uncertain<Sign>(n1 + EPS_dbl < n2 ? SMALLER : (n1 > n2 + EPS_dbl ? LARGER : EQUAL)) : Uncertain<Sign>::indeterminate();
}
inline Uncertain<Sign> certified_compare(const Rational& x, const Rational& y)
{
	Uncertain<Sign> r = Uncertain<Sign>::indeterminate();
	Uncertain<Sign> xnumsign = certified_sign(x.n());
	Uncertain<Sign> xdensign = certified_sign(x.d());
	Uncertain<Sign> ynumsign = certified_sign(y.n());
	Uncertain<Sign> ydensign = certified_sign(y.d());
	if (is_certain(xnumsign) && is_certain(xdensign) && is_certain(ynumsign) && is_certain(ydensign))
	{
		int xsign = xnumsign * xdensign;
		int ysign = ynumsign * ydensign;
		if (xsign == 0) return static_cast<Sign>(-ysign);
		if (ysign == 0) return static_cast<Sign>(xsign);
		// now (x != 0) && (y != 0)
		int diff = xsign - ysign;
		if (diff == 0)
		{
			int msign = xdensign * ydensign;
			double leftop = x.n() * y.d() * msign;
			double rightop = y.n() * x.d() * msign;
			r = certified_compare(leftop, rightop);
		}
		else
		{
			r = (xsign < ysign) ? SMALLER : LARGER;
		}
	}

	return r;
}

inline Uncertain<bool> certified_is_smaller_or_equal(Uncertain<Sign> c)
{
	return c == SMALLER || c == EQUAL;
}
inline Uncertain<bool> certified_is_smaller_or_equal(const Rational& n1, const Rational& n2)
{
	return certified_is_smaller_or_equal(certified_compare(n1, n2));
}
template<typename T>
inline Uncertain<bool> certified_is_equal(const T& n1, const T& n2)
{
	return certified_compare(n1, n2) == EQUAL;
}
template<typename T>
inline Uncertain<bool> certified_is_smaller(const T& n1, const T& n2)
{
	return certified_compare(n1, n2) == SMALLER;
}
template<typename T>
inline Uncertain<bool> certified_is_larger(const T& n1, const T& n2)
{
	return certified_compare(n1, n2) == LARGER;
}
inline Sign compare(const double &px, const double &py)
{
	return px + EPS_dbl < py ? SMALLER : (px > py + EPS_dbl ? LARGER : EQUAL);
}
inline Sign compare_lexicographically_xyC2(const double &px, const double &py, const double &qx, const double &qy)
{
	Sign c = compare(px, qx);
	return (c != EQUAL) ? c : compare(py, qy);
}
inline double determinant(const double& a00, const double& a01, const double& a10, const double& a11)
{
	return a00 * a11 - a10 * a01;
}
inline Sign sign_of_determinant(const double& a00, const double& a01, const double& a10, const double& a11)
{
	return compare(a00*a11, a10*a01);
}
inline Uncertain<Sign> certified_sign_of_determinant2x2(const double& a00, const double& a01, const double& a10, const double& a11)
{
	return certified_compare(a00*a11, a10*a01);
}
inline Sign operator-(Sign o)
{
	return static_cast<Sign>(-static_cast<int>(o));
}
inline Sign compare_angle_with_x_axisC2(const double &dx1, const double &dy1, const double &dx2, const double &dy2)
{
	// angles are in [-pi,pi], and the angle between Ox and d1 is compared
	// with the angle between Ox and d2
	int quadrant_1 = (dx1 >= 0) ? (dy1 >= 0 ? 1 : 4) : (dy1 >= 0 ? 2 : 3);
	int quadrant_2 = (dx2 >= 0) ? (dy2 >= 0 ? 1 : 4) : (dy2 >= 0 ? 2 : 3);
	if (quadrant_1 > quadrant_2) return LARGER;
	if (quadrant_1 < quadrant_2) return SMALLER;
	return -sign_of_determinant(dx1, dy1, dx2, dy2);
}

inline double square(double x) { return x * x; }

class Point2D
{
public:
	static Point2D Empty;
	Point2D() {}
	Point2D(const Point2D& var1)
	{
		x = var1.x;
		y = var1.y;
	}
	Point2D(double var1, double var3)
	{
		x = var1;
		y = var3;
	}
	void Negate()
	{
		x = -x;
		y = -y;
	}
	double DistanceTo(const Point2D& var1) const
	{
		double var2 = x - var1.x;
		double var4 = y - var1.y;
		return sqrt(var2 * var2 + var4 * var4);
	}
	Point2D Normalized()
	{
		double var1 = 1.0 / sqrt(x*x + y * y);
		x *= var1;
		y *= var1;
		Point2D V(x, y);
		return V;
	}
	double Dot(const Point2D& var1) const
	{
		return x * var1.x + y * var1.y;
	}
	double Perp(const Point2D& v) const
	{
		return x * v.y - y * v.x;
	}
	double DistanceSquared(const Point2D& var1) const
	{
		double var2 = x - var1.x;
		double var4 = y - var1.y;
		return var2 * var2 + var4 * var4;
	}
	Point2D operator+() const
	{
		Point2D V(x, y);
		return V;
	}
	Point2D operator+(const Point2D& right) const
	{
		Point2D V(x + right.x, y + right.y);
		return V;
	}
	Point2D& operator+=(const Point2D& right)
	{
		x += right.x;
		y += right.y;
		return *this;
	}
	Point2D operator-() const
	{
		Point2D V(-x, -y);
		return V;
	}
	Point2D operator-(const Point2D& right) const
	{
		Point2D V(x - right.x, y - right.y);
		return V;
	}
	Point2D& operator-=(const Point2D& right)
	{
		x -= right.x;
		y -= right.y;
		return *this;
	}
	Point2D operator*(double scale) const
	{
		Point2D V(x * scale, y * scale);
		return V;
	}
	double operator*(const Point2D& right)const
	{
		return x * right.x + y * right.y;
	}
	bool operator==(const Point2D& right) const
	{
		return Equals(right);
	}
	bool operator!=(const Point2D& right) const
	{
		return !Equals(right);
	}
	bool Equals(const Point2D& obj) const
	{
		return x == obj.x && y == obj.y;
	}
	bool operator<(const Point2D &d) const
	{
		return compare_angle_with_x_axisC2(x,y,d.x,d.y) == SMALLER;
	}
	bool operator>(const Point2D &d) const
	{
		return d < *this;
	}
	bool operator>=(const Point2D &d) const
	{
		return compare_angle_with_x_axisC2(x, y, d.x, d.y) != SMALLER;
	}
	bool operator<=(const Point2D &d) const
	{
		return compare_angle_with_x_axisC2(x, y, d.x, d.y) != LARGER;
	}
	double x;
	double y;
};
inline double squared_distance(const Point2D& a, const Point2D& b)
{
	Point2D d = b - a;
	return d.x*d.x + d.y*d.y;
}
inline Point2D midpoint(const Point2D& a, const Point2D& b)
{
	return (a + b) * 0.5;
}
inline Sign compare_xy(const Point2D& pa, const Point2D& pb)
{
	return compare_lexicographically_xyC2(pa.x, pa.y, pb.x, pb.y);
}
struct Line
{
	double a;
	double b;
	double c;
	Line() {}
	Line(const Point2D& pP1, const Point2D& pP2)
	{
		a = pP1.y - pP2.y;
		b = pP2.x - pP1.x;
		c = pP1.x*pP2.y - pP2.x*pP1.y;
	}
	Line(double pA, double pB, double pC)
	{
		a = pA;
		b = pB;
		c = pC;
	}
};
class HalfEdge;
class Vertex;
class Face
{
public:
	Face() : mID(-1), mHE(NULL) {}
	Face(int aID) : mID(aID), mHE(NULL) {}
public:
	int id() const { return mID; }
	HalfEdge* halfedge() { return mHE; }
	HalfEdge* halfedge() const { return mHE; }
	void set_halfedge(HalfEdge* aHE) { mHE = aHE; }
	void reset_id(int aID) { mID = aID; }
private:
	int mID;
	HalfEdge* mHE;
};
class HalfEdge
{
public:
	HalfEdge() : mOpp(NULL), mNxt(NULL), mPrv(NULL), mV(NULL), mF(NULL), mID(-1), mSlope(0) {}
	HalfEdge(int aID) : mOpp(NULL), mNxt(NULL), mPrv(NULL), mV(NULL), mF(NULL), mID(aID), mSlope(0) {}
	HalfEdge(int aID, int aSlope) : mOpp(NULL), mNxt(NULL), mPrv(NULL), mV(NULL), mF(NULL), mID(aID), mSlope(aSlope) {}
public:
	int id() const { return mID; }
	bool is_bisector() const { return !this->is_border() && !this->opposite()->is_border(); }
	bool is_inner_bisector() const;
	bool has_null_segment() const;
	bool has_infinite_time() const;
	HalfEdge* defining_contour_edge() const { return this->face()->halfedge(); }
	HalfEdge* defining_contour_edge() { return this->face()->halfedge(); }
	HalfEdge* opposite() { return mOpp; }
	HalfEdge* opposite() const { return mOpp; }
	HalfEdge* next() { return mNxt; }
	HalfEdge* next() const { return mNxt; }
	HalfEdge* prev() { return mPrv; }
	HalfEdge* prev() const { return mPrv; }
	Vertex* vertex() { return mV; }
	Vertex* vertex() const { return mV; }
	Face* face() { return mF; }
	Face* face() const { return mF; }
	int slope() const { return mSlope; }
	bool is_border() const { return mF == NULL; }
	void set_opposite(HalfEdge* h) { mOpp = h; }
	void set_next(HalfEdge* h) { mNxt = h; }
	void set_prev(HalfEdge* h) { mPrv = h; }
	void set_vertex(Vertex*   w) { mV = w; }
	void set_face(Face*     g) { mF = g; }
	void set_slope(int aSlope) { mSlope = aSlope; }
	void reset_id(int aID) { mID = aID; }
private:
	HalfEdge* mOpp;
	HalfEdge* mNxt;
	HalfEdge* mPrv;
	Vertex* mV;
	Face* mF;
	int mID;
	int mSlope;
};

class Triedge
{
public:
	Triedge()
	{
		mE[0] = NULL;
		mE[1] = NULL;
		mE[2] = NULL;
	}
	Triedge(HalfEdge* aE0, HalfEdge* aE1)
	{
		mE[0] = aE0;
		mE[1] = aE1;
		mE[2] = NULL;
	}
	Triedge(HalfEdge* aE0, HalfEdge* aE1, HalfEdge* aE2)
	{
		mE[0] = aE0;
		mE[1] = aE1;
		mE[2] = aE2;
	}
	HalfEdge* e(unsigned idx) const { return mE[idx]; }
	HalfEdge* e0() const { return e(0); }
	HalfEdge* e1() const { return e(1); }
	HalfEdge* e2() const { return e(2); }
	bool is_valid() const { return e0()!=NULL && e1()!=NULL; }
	bool is_contour() const { return e2()==NULL; }
	bool is_skeleton() const { return  e2()!=NULL; }
	bool is_contour_terminal() const { return e0() == e1(); }
	bool is_skeleton_terminal() const { return e0() == e1() || e1() == e2(); }
	// Returns true if the triedges store the same 3 halfedges (in any order)
	friend bool operator == (Triedge const& x, Triedge const& y)
	{
		return x.number_of_unique_edges() == y.number_of_unique_edges() && CountInCommon(x, y) == x.number_of_unique_edges();
	}
	friend bool operator != (Triedge const& x, Triedge const& y) { return !(x == y); }
	friend Triedge operator & (Triedge const& x, Triedge const& y)
	{
		return Triedge(x.e0(), x.e1(), (x.e0() == y.e0() || x.e1() == y.e0()) ? y.e1() : y.e0());
	}
private:
	// returns 1 if aE is one of the halfedges stored in this triedge, 0 otherwise.
	int contains(HalfEdge* aE) const
	{
		return aE == e0() || aE == e1() || aE == e2() ? 1 : 0;
	}
	int number_of_unique_edges() const
	{
		return is_contour() ? (is_contour_terminal() ? 1 : 2) : (is_skeleton_terminal() ? 2 : 3);
	}
	// Returns the number of common halfedges in the two triedges x and y
	static int CountInCommon(Triedge const& x, Triedge const& y)
	{
		HalfEdge* lE[3];
		int lC = 1;
		lE[0] = y.e0();
		if (y.e0() != y.e1())
			lE[lC++] = y.e1();
		if (y.e0() != y.e2() && y.e1() != y.e2())
			lE[lC++] = y.e2();
		return x.contains(lE[0]) + x.contains(lE[1]) + (lC > 2 ? x.contains(lE[2]) : 0);
	}
	HalfEdge* mE[3];
};

class Vertex
{
	enum Flags { IsSplitBit = 0x01, HasInfiniteTimeBit = 0x02 };
protected:
	class HalfEdgeCirculator
	{
	public:
		HalfEdgeCirculator() : mHandle(NULL) {}
		HalfEdgeCirculator(HalfEdge* aHandle) : mHandle(aHandle) {}
		bool operator==(HalfEdgeCirculator& Other) const
		{
			return mHandle == Other.mHandle;
		}
		bool operator!=(HalfEdgeCirculator& Other) const { return mHandle!=Other.mHandle; }
		HalfEdgeCirculator& operator++()
		{
			mHandle = mHandle->opposite()->prev();
			return *this;
		}
	private:
		void increment() { mHandle = mHandle->opposite()->prev(); }
		void decrement() { mHandle = mHandle->next()->opposite(); }
		HalfEdge* mHandle;
	};
public:
	Vertex() : mID(-1), mHE(NULL), mTime(0.0), mFlags(0) {}
	// Infinite vertex
	Vertex(int aID) :mID(aID), mHE(NULL), mP(0, 0), mTime(MAX_dbl), mFlags(HasInfiniteTimeBit) {}
	// Contour vertex
	Vertex(int aID, Point2D const& aP) :mID(aID), mHE(NULL), mP(aP), mTime(0.0), mFlags(0) {}
	// Skeleton vertex, corresponding to a split or edge event.
	Vertex(int aID, Point2D const& aP, double aTime, bool aIsSplit, bool aHasInfiniteTime) :mID(aID), mHE(NULL), mP(aP), mTime(aTime), mFlags((aIsSplit ? IsSplitBit : 0) | (aHasInfiniteTime ? HasInfiniteTimeBit : 0)) {}
public:
	int id() const { return mID; }
	double time() const { return mTime; }
	bool has_infinite_time() const { return (mFlags & HasInfiniteTimeBit) == HasInfiniteTimeBit; }
	bool has_null_point() const { return has_infinite_time(); }
	bool is_split() const { return (mFlags & IsSplitBit) == IsSplitBit; }
	HalfEdge* primary_bisector() const { return halfedge()->next(); }
	HalfEdge* primary_bisector() { return halfedge()->next(); }
	HalfEdgeCirculator halfedge_around_vertex_begin() const { return HalfEdgeCirculator(halfedge()); }
	HalfEdgeCirculator halfedge_around_vertex_begin() { return HalfEdgeCirculator(halfedge()); }
	size_t degree() const
	{
		HalfEdgeCirculator begin = halfedge_around_vertex_begin();
		HalfEdgeCirculator it = begin;
		size_t size = 0;
		do
		{
			++size;
			++it;
		} while (it != begin);
		return size;
	}
	bool is_skeleton() const { return  halfedge()->is_bisector(); }
	bool is_contour() const { return !halfedge()->is_bisector(); }
	const Point2D& point() const { return mP; }
	HalfEdge* halfedge() { return mHE; }
	HalfEdge* halfedge() const { return mHE; }
	void set_halfedge(HalfEdge* aHE) { mHE = aHE; }
	Triedge const& event_triedge() const { return mEventTriedge; }
	void set_event_triedge(Triedge const& aTriedge) { mEventTriedge = aTriedge; }
	void reset_id__internal__(int aID) { mID = aID; }
	void reset_point__internal__(Point2D const& aP) { mP = aP; }
private:
	int mID;
	HalfEdge* mHE;
	Triedge mEventTriedge;
	Point2D mP;
	double mTime;
	unsigned char mFlags;
};
class Segment
{
public:
	Segment() {}
	Segment(const Point2D &sp, const Point2D &ep) :start(sp), end(ep) {}
	Point2D start;
	Point2D end;
};
enum Trisegment_collinearity
{
	TRISEGMENT_COLLINEARITY_NONE,
	TRISEGMENT_COLLINEARITY_01,
	TRISEGMENT_COLLINEARITY_12,
	TRISEGMENT_COLLINEARITY_02,
	TRISEGMENT_COLLINEARITY_ALL
};
template<>
inline Uncertain<Trisegment_collinearity> Uncertain<Trisegment_collinearity>::indeterminate()
{
	return Uncertain<Trisegment_collinearity>(TRISEGMENT_COLLINEARITY_NONE, TRISEGMENT_COLLINEARITY_ALL);
}
class Trisegment
{
public:
	Trisegment(Segment const& aE0, Segment const& aE1, Segment const& aE2, Trisegment_collinearity aCollinearity) :mChildL(NULL), mChildR(NULL)
	{
		mCollinearity = aCollinearity;
		mE[0] = aE0;
		mE[1] = aE1;
		mE[2] = aE2;
		switch (mCollinearity)
		{
		case TRISEGMENT_COLLINEARITY_01:
			mCSIdx = 0; mNCSIdx = 2; break;
		case TRISEGMENT_COLLINEARITY_12:
			mCSIdx = 1; mNCSIdx = 0; break;
		case TRISEGMENT_COLLINEARITY_02:
			mCSIdx = 0; mNCSIdx = 1; break;
		case TRISEGMENT_COLLINEARITY_ALL:
			mCSIdx = mNCSIdx = 0xFFFFFFFF; break;
		case TRISEGMENT_COLLINEARITY_NONE:
			mCSIdx = mNCSIdx = 0xFFFFFFFF; break;
		}
	}
	Trisegment_collinearity collinearity() const { return mCollinearity; }
	Segment const& e(unsigned idx) const { return mE[idx]; }
	Segment const& e0() const { return e(0); }
	Segment const& e1() const { return e(1); }
	Segment const& e2() const { return e(2); }
	// If 2 out of the 3 edges are collinear they can be reclassified as 1 collinear edge (any of the 2) and 1 non-collinear.
	// These methods returns the edges according to that classification.
	// PRECONDITION: Exactly 2 out of 3 edges are collinear
	Segment const& collinear_edge() const { return e(mCSIdx); }
	Segment const& non_collinear_edge() const { return e(mNCSIdx); }
	Trisegment* child_l() const { return mChildL; }
	Trisegment* child_r() const { return mChildR; }
	void set_child_l(Trisegment* const& aChild) { mChildL = aChild; }
	void set_child_r(Trisegment* const& aChild) { mChildR = aChild; }
	enum SEED_ID { LEFT, RIGHT, UNKNOWN };
	// Indicates which of the seeds is collinear for a normal collinearity case.
	// PRECONDITION: The collinearity is normal.
	SEED_ID degenerate_seed_id() const
	{
		Trisegment_collinearity c = collinearity();
		return c == TRISEGMENT_COLLINEARITY_01 ? LEFT : c == TRISEGMENT_COLLINEARITY_12 ? RIGHT : UNKNOWN;
	}
private:
	Segment mE[3];
	Trisegment_collinearity mCollinearity;
	unsigned mCSIdx, mNCSIdx;
	Trisegment* mChildL;
	Trisegment* mChildR;
};

class Event
{
public:
	enum Type { cEdgeEvent, cSplitEvent, cPseudoSplitEvent };
public:
	Event(Triedge const& aTriedge, Trisegment* aTrisegment) :mTriedge(aTriedge), mTrisegment(aTrisegment) {}
	virtual ~Event() {}
	virtual Type type() const = 0;
	virtual Vertex* seed0() const = 0;
	virtual Vertex* seed1() const = 0;
	Triedge const& triedge() const { return mTriedge; }
	Trisegment* const& trisegment() const { return mTrisegment; }
	Point2D const& point() const { return mP; }
	double time() const { return mTime; }
	void SetTimeAndPoint(double aTime, Point2D const& aP) { mTime = aTime; mP = aP; }
private:
	Triedge mTriedge;
	Trisegment* mTrisegment;
	Point2D mP;
	double mTime;
};

class EdgeEvent : public Event
{
public:
	EdgeEvent(Triedge const& aTriedge, Trisegment* aTrisegment, Vertex* aLSeed, Vertex* aRSeed) :Event(aTriedge, aTrisegment), mLSeed(aLSeed), mRSeed(aRSeed) {}
	virtual Type type() const { return this->cEdgeEvent; }
	virtual Vertex* seed0() const { return mLSeed; }
	virtual Vertex* seed1() const { return mRSeed; }
private:
	Vertex* mLSeed;
	Vertex* mRSeed;
};

class SplitEvent : public Event
{
public:
	SplitEvent(Triedge const& aTriedge, Trisegment* aTrisegment, Vertex* aSeed) : Event(aTriedge, aTrisegment), mSeed(aSeed), mOppR(NULL) {}
	virtual Type type() const { return this->cSplitEvent; }
	virtual Vertex* seed0() const { return mSeed; }
	virtual Vertex* seed1() const { return mSeed; }
	void set_opposite_rnode(Vertex* aOppR) { mOppR = aOppR; }
	Vertex* opposite_rnode() const { return mOppR; }
private:
	Vertex* mSeed;
	Vertex* mOppR;
};

class PseudoSplitEvent : public Event
{
public:
	PseudoSplitEvent(Triedge const& aTriedge, Trisegment* aTrisegment, Vertex* aSeed0, Vertex* aSeed1, bool aOppositeIs0) :Event(aTriedge, aTrisegment), mSeed0(aSeed0), mSeed1(aSeed1), mOppositeIs0(aOppositeIs0) {}
	virtual Type type() const { return this->cPseudoSplitEvent; }
	virtual Vertex* seed0() const { return mSeed0; }
	virtual Vertex* seed1() const { return mSeed1; }
	bool opposite_node_is_seed_0() const { return mOppositeIs0; }
	bool is_at_source_vertex() const { return opposite_node_is_seed_0(); }
	Vertex* opposite_seed() const { return opposite_node_is_seed_0() ? seed0() : seed1(); }
private:
	Vertex* mSeed0;
	Vertex* mSeed1;
	bool mOppositeIs0;
};
typedef std::vector<Vertex*>::iterator VertexIterator;
typedef std::vector<HalfEdge*>::iterator HalfEdgeIterator;
typedef std::vector<Face*>::iterator FaceIterator;
typedef std::pair<Vertex*, Vertex*> VertexPair;
class SSkeleton
{
public:
	VertexIterator vertices_begin() { return vertices.begin(); };
	VertexIterator vertices_end() { return vertices.end(); }
	HalfEdgeIterator halfedges_begin() { return halfedges.begin(); }
	HalfEdgeIterator halfedges_end() { return halfedges.end(); }
	FaceIterator faces_begin() { return faces.begin(); };
	FaceIterator faces_end() { return faces.end(); }
	size_t size_of_vertices() const { return vertices.size(); }
	size_t size_of_halfedges() const { return halfedges.size(); }
	size_t size_of_faces() const { return faces.size(); }
	Vertex* vertices_push_back(Vertex* v)
	{
		vertices.push_back(v);
		return v;
	}
	HalfEdge* edges_push_back(HalfEdge* h, HalfEdge* g)
	{
		h->set_opposite(g);
		g->set_opposite(h);
		halfedges.push_back(h);
		halfedges.push_back(g);
		return h;
	}
	HalfEdge* edges_push_back(HalfEdge* h)
	{
		return edges_push_back(h, h->opposite());
	}
	Face* faces_push_back(Face* f)
	{
		faces.push_back(f);
		return f;
	}
	void vertices_erase(Vertex* v)
	{
		vertices.erase(std::find(vertices.begin(), vertices.end(), v));
	}
	void edges_erase(HalfEdge* h)
	{
		// deletes the pair of opposite halfedges h and h->opposite().
		HalfEdge* g = h->opposite();
		halfedges.erase(std::find(halfedges.begin(), halfedges.end(), h));
		halfedges.erase(std::find(halfedges.begin(), halfedges.end(), g));
	}
	bool is_valid();
	std::vector<Vertex*> vertices;
	std::vector<HalfEdge*> halfedges;
	std::vector<Face*> faces;
};

std::optional<Line> compute_normalized_line_ceoffC2(Segment const& e);
std::optional<Rational> compute_normal_offset_lines_isec_timeC2(Trisegment* const& tri);
std::optional<Point2D> construct_normal_offset_lines_isecC2(Trisegment* const& tri);
Uncertain<bool> are_edges_parallelC2(Segment const& e0, Segment const& e1);
Uncertain<bool> are_parallel_edges_equally_orientedC2(Segment const& e0, Segment const& e1);
Uncertain<bool> certified_collinearC2(Point2D const& p, Point2D const& q, Point2D const& r);
Uncertain<bool> are_edges_collinearC2(Segment const& e0, Segment const& e1);
Uncertain<bool> are_edges_orderly_collinearC2(Segment const& e0, Segment const& e1);
Uncertain<Trisegment_collinearity> certified_trisegment_collinearity(Segment const& e0, Segment const& e1, Segment const& e2);
void line_project_pointC2(const double &la, const double &lb, const double &lc, const double &px, const double &py, double &x, double &y);
std::optional<Point2D> compute_degenerate_seed_pointC2(Trisegment* const& tri);
std::optional<Point2D> construct_degenerate_offset_lines_isecC2(Trisegment* const& tri);
std::optional<Point2D> construct_offset_lines_isecC2(Trisegment* const& tri);
std::optional<Point2D> compute_oriented_midpoint(Segment const& e0, Segment const& e1);
std::optional<Point2D> compute_seed_pointC2(Trisegment* const& tri, Trisegment::SEED_ID sid);
std::optional<Rational> compute_degenerate_offset_lines_isec_timeC2(Trisegment* const& tri);
std::optional<Rational> compute_offset_lines_isec_timeC2(Trisegment* const& tri);
Uncertain<bool> exist_offset_lines_isec2(Trisegment* const& tri, std::optional<double> aMaxTime);
bool Do_ss_event_exist_2(Trisegment* const& aTrisegment, std::optional<double> aMaxTime);
void line_from_pointsC2(const double &px, const double &py, const double &qx, const double &qy, double &a, double &b, double &c);
Uncertain<Sign> certified_side_of_oriented_lineC2(const double &a, const double &b, const double &c, const double &x, const double &y);
Uncertain<bool> is_edge_facing_pointC2(std::optional<Point2D> const& aP, Segment const& aEdge);
inline Uncertain<bool> is_edge_facing_offset_lines_isecC2(Trisegment* const& tri, Segment const& aEdge);
Uncertain<bool> Is_edge_facing_ss_node_2(Point2D const& aContourNode, Segment const& aEdge);
Uncertain<bool> Is_edge_facing_ss_node_2(Trisegment* const& aSkeletonNode, Segment const& aEdge);
void perpendicular_through_pointC2(const double &la, const double &lb, const double &px, const double &py, double &a, double &b, double &c);
Uncertain<Sign> oriented_side_of_event_point_wrt_bisectorC2(Trisegment* const& event, Segment const& e0, Segment const& e1, Trisegment* const& v01_event, bool primary_is_0);
Uncertain<Sign> compare_offset_lines_isec_timesC2(Trisegment* const& m, Trisegment* const& n);
Uncertain<bool> are_events_simultaneousC2(Trisegment* const& l, Trisegment* const& r);
Sign orientation(const Point2D &p, const Point2D &q, const Point2D &r);
Sign orientation(const Point2D &p, const Point2D &q);
std::optional<std::pair<double, Point2D>> Construct_ss_event_time_and_point_2(Trisegment* const& aTrisegment);