// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "SSkeleton.h"
#pragma float_control(precise, on, push)

std::optional<Line> compute_normalized_line_ceoffC2(Segment const& e)
{
	bool finite = true;
	double a(0.0), b(0.0), c(0.0);
	if (e.start.y == e.end.y)
	{
		a = 0;
		if (e.end.x > e.start.x)
		{
			b = 1;
			c = -e.start.y;
		}
		else if (e.end.x == e.start.x)
		{
			b = 0;
			c = 0;
		}
		else
		{
			b = -1;
			c = e.start.y;
		}
	}
	else if (e.end.x == e.start.x)
	{
		b = 0;
		if (e.end.y > e.start.y)
		{
			a = -1;
			c = e.start.x;
		}
		else if (e.end.y == e.start.y)
		{
			a = 0;
			c = 0;
		}
		else
		{
			a = 1;
			c = -e.start.x;
		}
	}
	else
	{
		double sa = e.start.y - e.end.y;
		double sb = e.end.x - e.start.x;
		double l2 = (sa*sa) + (sb*sb);
		if (std::isfinite(l2))
		{
			double l = sqrt(l2);
			a = sa / l;
			b = sb / l;
			c = -e.start.x*a - e.start.y*b;
		}
		else finite = false;
	}
	if (finite)
		if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c))
			finite = false;
	return finite ? std::optional<Line>(Line(a, b, c)) : std::optional<Line>();
}
std::optional<Rational> compute_normal_offset_lines_isec_timeC2(Trisegment* const& tri)
{
	double num(0.0), den(0.0);
	bool ok = false;
	std::optional<Line> l0 = compute_normalized_line_ceoffC2(tri->e0());
	std::optional<Line> l1 = compute_normalized_line_ceoffC2(tri->e1());
	std::optional<Line> l2 = compute_normalized_line_ceoffC2(tri->e2());
	if (l0 && l1 && l2)
	{
		num = (l2->a*l0->b*l1->c)
			- (l2->a*l1->b*l0->c)
			- (l2->b*l0->a*l1->c)
			+ (l2->b*l1->a*l0->c)
			+ (l1->b*l0->a*l2->c)
			- (l0->b*l1->a*l2->c);
		den = (-l2->a*l1->b)
			+ (l2->a*l0->b)
			+ (l2->b*l1->a)
			- (l2->b*l0->a)
			+ (l1->b*l0->a)
			- (l0->b*l1->a);
		ok = std::isfinite(num) && std::isfinite(den);
	}
	return ok ? std::optional<Rational>(Rational(num, den)) : std::optional<Rational>();
}
std::optional<Point2D> construct_normal_offset_lines_isecC2(Trisegment* const& tri)
{
	double x(0.0), y(0.0);
	std::optional<Line> l0 = compute_normalized_line_ceoffC2(tri->e0());
	std::optional<Line> l1 = compute_normalized_line_ceoffC2(tri->e1());
	std::optional<Line> l2 = compute_normalized_line_ceoffC2(tri->e2());
	bool ok = false;
	if (l0 && l1 && l2)
	{
		double den = l0->a*l2->b - l0->a*l1->b - l1->a*l2->b + l2->a*l1->b + l0->b*l1->a - l0->b*l2->a;
		if (!certified_is_zero(den))
		{
			double numX = l0->b*l2->c - l0->b*l1->c - l1->b*l2->c + l2->b*l1->c + l1->b*l0->c - l2->b*l0->c;
			double numY = l0->a*l2->c - l0->a*l1->c - l1->a*l2->c + l2->a*l1->c + l1->a*l0->c - l2->a*l0->c;
			if (std::isfinite(den) && std::isfinite(numX) && std::isfinite(numY))
			{
				ok = true;
				x = numX / den;
				y = -numY / den;
			}
		}
	}
	return ok ? std::optional<Point2D>(Point2D(x, y)) : std::optional<Point2D>();
}
Uncertain<bool> are_edges_parallelC2(Segment const& e0, Segment const& e1)
{
	Uncertain<Sign> s = certified_sign_of_determinant2x2(e0.end.x - e0.start.x, e0.end.y - e0.start.y, e1.end.x - e1.start.x, e1.end.y - e1.start.y);
	return (s == Uncertain<Sign>(ZERO));
}
Uncertain<bool> are_parallel_edges_equally_orientedC2(Segment const& e0, Segment const& e1)
{
	return certified_sign((e0.end - e0.start) * (e1.end - e1.start)) == POSITIVE;
}
Uncertain<bool> certified_collinearC2(Point2D const& p, Point2D const& q, Point2D const& r)
{
	return certified_is_equal((q.x - p.x) * (r.y - p.y), (r.x - p.x) * (q.y - p.y));
}
Uncertain<bool> are_edges_collinearC2(Segment const& e0, Segment const& e1)
{
	return certified_collinearC2(e0.start, e0.end, e1.start) & certified_collinearC2(e0.start, e0.end, e1.end);
}
Uncertain<bool> are_edges_orderly_collinearC2(Segment const& e0, Segment const& e1)
{
	return are_edges_collinearC2(e0, e1) & are_parallel_edges_equally_orientedC2(e0, e1);
}
Uncertain<Trisegment_collinearity> certified_trisegment_collinearity(Segment const& e0, Segment const& e1, Segment const& e2)
{
	Uncertain<bool> is_01 = are_edges_orderly_collinearC2(e0, e1);
	if (is_certain(is_01))
	{
		Uncertain<bool> is_02 = are_edges_orderly_collinearC2(e0, e2);
		if (is_certain(is_02))
		{
			Uncertain<bool> is_12 = are_edges_orderly_collinearC2(e1, e2);
			if (is_certain(is_12))
			{
				if (is_01 && !is_02 && !is_12) return TRISEGMENT_COLLINEARITY_01;
				if (is_02 && !is_01 && !is_12) return TRISEGMENT_COLLINEARITY_02;
				if (is_12 && !is_01 && !is_02) return TRISEGMENT_COLLINEARITY_12;
				if (!is_01 && !is_02 && !is_12) return TRISEGMENT_COLLINEARITY_NONE;
				return TRISEGMENT_COLLINEARITY_ALL;
			}
		}
	}
	return Uncertain<Trisegment_collinearity>::indeterminate();
}
void line_project_pointC2(const double &la, const double &lb, const double &lc, const double &px, const double &py, double &x, double &y)
{
	if (certainly(is_zero(la))) // horizontal line
	{
		x = px;
		y = -lc / lb;
	}
	else if (certainly(is_zero(lb))) // vertical line
	{
		x = -lc / la;
		y = py;
	}
	else
	{
		double a2 = square(la);
		double b2 = square(lb);
		double d = a2 + b2;
		x = (b2*px - la * lb*py - la * lc) / d;
		y = (-la * lb*px + a2 * py - lb * lc) / d;
	}
}
std::optional<Point2D> construct_degenerate_offset_lines_isecC2(Trisegment* const& tri)
{
	double x(0.0), y(0.0);
	std::optional<Line> l0 = compute_normalized_line_ceoffC2(tri->collinear_edge());
	std::optional<Line> l2 = compute_normalized_line_ceoffC2(tri->non_collinear_edge());
	std::optional<Point2D> q = compute_degenerate_seed_pointC2(tri);
	bool ok = false;
	if (l0 && l2 && q)
	{
		double num, den;
		double px, py;
		line_project_pointC2(l0->a, l0->b, l0->c, q->x, q->y, px, py);
		if (!is_zero(l0->b)) // Non-vertical
		{
			num = (l2->a * l0->b - l0->a * l2->b) * px + l0->b * l2->c - l2->b * l0->c;
			den = (l0->a * l0->a - 1) * l2->b + (1 - l2->a * l0->a) * l0->b;
		}
		else
		{
			num = (l2->a * l0->b - l0->a * l2->b) * py - l0->a * l2->c + l2->a * l0->c;
			den = l0->a * l0->b * l2->b - l0->b * l0->b * l2->a + l2->a - l0->a;
		}
		if (!certified_is_zero(den) && std::isfinite(den) && std::isfinite(num))
		{
			x = px + l0->a * num / den;
			y = py + l0->b * num / den;
			ok = std::isfinite(x) && std::isfinite(y);
		}
	}
	return ok ? std::optional<Point2D>(Point2D(x, y)) : std::optional<Point2D>();
}
std::optional<Point2D> construct_offset_lines_isecC2(Trisegment* const& tri)
{
	return tri->collinearity() == TRISEGMENT_COLLINEARITY_NONE ? construct_normal_offset_lines_isecC2(tri) : construct_degenerate_offset_lines_isecC2(tri);
}
std::optional<Point2D> compute_oriented_midpoint(Segment const& e0, Segment const& e1)
{
	bool ok = false;
	double delta01 = squared_distance(e0.end, e1.start);
	double delta10 = squared_distance(e1.end, e0.start);
	Point2D mp;
	if (std::isfinite(delta01) && std::isfinite(delta10))
	{
		if (delta01 <= delta10)
			mp = midpoint(e0.end, e1.start);
		else mp = midpoint(e1.end, e0.start);
		ok = std::isfinite(mp.x) && std::isfinite(mp.y);
	}
	return ok ? std::optional<Point2D>(mp) : std::optional<Point2D>();
}
std::optional<Point2D> compute_seed_pointC2(Trisegment* const& tri, Trisegment::SEED_ID sid)
{
	std::optional<Point2D> p;
	switch (sid)
	{
	case Trisegment::LEFT:
		p = tri->child_l() ? construct_offset_lines_isecC2(tri->child_l())  // this can recurse
			: compute_oriented_midpoint(tri->e0(), tri->e1());
		break;
	case Trisegment::RIGHT:
		p = tri->child_r() ? construct_offset_lines_isecC2(tri->child_r()) // this can recurse
			: compute_oriented_midpoint(tri->e1(), tri->e2());
		break;
	case Trisegment::UNKNOWN:
		p = compute_oriented_midpoint(tri->e0(), tri->e2());
		break;
	}
	return p;
}
std::optional<Point2D> compute_degenerate_seed_pointC2(Trisegment* const& tri)
{
	return compute_seed_pointC2(tri, tri->degenerate_seed_id());
}
std::optional<Rational> compute_degenerate_offset_lines_isec_timeC2(Trisegment* const& tri)
{
	bool ok = false;
	std::optional<Line> l0 = compute_normalized_line_ceoffC2(tri->collinear_edge());
	std::optional<Line> l2 = compute_normalized_line_ceoffC2(tri->non_collinear_edge());
	std::optional<Point2D> q = compute_degenerate_seed_pointC2(tri);
	double num(0.0), den(0.0);
	if (l0 && l2 && q)
	{
		double px, py;
		line_project_pointC2(l0->a, l0->b, l0->c, q->x, q->y, px, py);
		if (!is_zero(l0->b)) // Non-vertical
		{
			num = (l2->a * l0->b - l0->a * l2->b) * px + l0->b * l2->c - l2->b * l0->c;
			den = (l0->a * l0->a - 1) * l2->b + (1 - l2->a * l0->a) * l0->b;
		}
		else
		{
			num = (l2->a * l0->b - l0->a * l2->b) * py - l0->a * l2->c + l2->a * l0->c;
			den = l0->a * l0->b * l2->b - l0->b * l0->b * l2->a + l2->a - l0->a;
		}
		ok = std::isfinite(num) && std::isfinite(den);
	}
	return ok ? std::optional<Rational>(Rational(num, den)) : std::optional<Rational>();
}
std::optional<Rational> compute_offset_lines_isec_timeC2(Trisegment* const& tri)
{
	return tri->collinearity() == TRISEGMENT_COLLINEARITY_NONE ? compute_normal_offset_lines_isec_timeC2(tri) : compute_degenerate_offset_lines_isec_timeC2(tri);
}
Uncertain<bool> exist_offset_lines_isec2(Trisegment* const& tri, std::optional<double> aMaxTime)
{
	Uncertain<bool> rResult = Uncertain<bool>::indeterminate();
	if (tri->collinearity() != TRISEGMENT_COLLINEARITY_ALL)
	{
		std::optional<Rational> t = compute_offset_lines_isec_timeC2(tri);
		if (t)
		{
			Uncertain<bool> d_is_zero = certified_is_zero(t->d());
			if (is_certain(d_is_zero))
			{
				if (!d_is_zero)
				{
					rResult = certified_is_positive(t);
					if (aMaxTime && certainly(rResult))
						rResult = certified_is_smaller_or_equal(t, Rational(aMaxTime));
				}
				else rResult = false;
			}
		}
	}
	else rResult = false;
	return rResult;
}
bool Do_ss_event_exist_2(Trisegment* const& aTrisegment, std::optional<double> aMaxTime)
{
	return exist_offset_lines_isec2(aTrisegment, aMaxTime);
}
void line_from_pointsC2(const double &px, const double &py, const double &qx, const double &qy, double &a, double &b, double &c)
{
	// The horizontal and vertical line get a special treatment
	// in order to make the intersection code robust for doubles 
	if (py == qy)
	{
		a = 0;
		if (qx > px)
		{
			b = 1;
			c = -py;
		}
		else if (qx == px)
		{
			b = 0;
			c = 0;
		}
		else
		{
			b = -1;
			c = py;
		}
	}
	else if (qx == px)
	{
		b = 0;
		if (qy > py)
		{
			a = -1;
			c = px;
		}
		else if (qy == py)
		{
			a = 0;
			c = 0;
		}
		else
		{
			a = 1;
			c = -px;
		}
	}
	else
	{
		a = py - qy;
		b = qx - px;
		c = -px * a - py * b;
	}
}
inline Uncertain<Sign> certified_side_of_oriented_lineC2(const double &a, const double &b, const double &c, const double &x, const double &y)
{
	return certified_sign(a*x + b * y + c);
}
Uncertain<bool> is_edge_facing_pointC2(std::optional<Point2D> const& aP, Segment const& aEdge)
{
	Uncertain<bool> rResult = Uncertain<bool>::indeterminate();
	if (aP)
	{
		double a, b, c;
		line_from_pointsC2(aEdge.start.x, aEdge.start.y, aEdge.end.x, aEdge.end.y, a, b, c);
		rResult = certified_side_of_oriented_lineC2(a, b, c, aP->x, aP->y) == Uncertain<Sign>(POSITIVE);
	}
	return rResult;
}

// Given a triple of oriented straight line segments: (e0,e1,e2) such that their offsets
// at some distance intersects in a point (x,y), returns true if (x,y) is on the positive side of the line supporting aEdge
//
Uncertain<bool> is_edge_facing_offset_lines_isecC2(Trisegment* const& tri, Segment const& aEdge)
{
	return is_edge_facing_pointC2(construct_offset_lines_isecC2(tri), aEdge);
}
Uncertain<bool> Is_edge_facing_ss_node_2(Point2D const& aContourNode, Segment const& aEdge)
{
	return is_edge_facing_pointC2(std::optional<Point2D>(aContourNode), aEdge);
}

Uncertain<bool> Is_edge_facing_ss_node_2(Trisegment* const& aSkeletonNode, Segment const& aEdge)
{
	return is_edge_facing_offset_lines_isecC2(aSkeletonNode, aEdge);
}
void perpendicular_through_pointC2(const double &la, const double &lb, const double &px, const double &py, double &a, double &b, double &c)
{
	a = -lb;
	b = la;
	c = lb * px - la * py;
}
Uncertain<Sign> oriented_side_of_event_point_wrt_bisectorC2(Trisegment* const& event, Segment const& e0, Segment const& e1, Trisegment* const& v01_event, bool primary_is_0)
{
	Uncertain<Sign> rResult = Uncertain<Sign>::indeterminate();
	try
	{
		Point2D p = validate(construct_offset_lines_isecC2(event));
		Line l0 = validate(compute_normalized_line_ceoffC2(e0));
		Line l1 = validate(compute_normalized_line_ceoffC2(e1));
		// Degenerate bisector?   
		if (certainly(are_edges_parallelC2(e0, e1)))
		{
			Point2D v01 = v01_event ? validate(construct_offset_lines_isecC2(v01_event)) : e1.start;
			double a, b, c;
			perpendicular_through_pointC2(primary_is_0 ? l0.a : l1.a, primary_is_0 ? l0.b : l1.b, v01.x, v01.y, a, b, c);
			rResult = certified_side_of_oriented_lineC2(a, b, c, p.x, p.y);
		}
		else // Valid (non-degenerate) angular bisector
		{
			// Scale distance from to the lines.
			double sd_p_l0 = validate(l0.a * p.x + l0.b * p.y + l0.c);
			double sd_p_l1 = validate(l1.a * p.x + l1.b * p.y + l1.c);
			Uncertain<bool> equal = certified_is_equal(sd_p_l0, sd_p_l1);
			if (is_certain(equal))
			{
				if (equal)
				{
					rResult = ON_ORIENTED_BOUNDARY;
				}
				else
				{
					Uncertain<bool> smaller = certified_is_smaller(validate(l0.a*l1.b), validate(l1.a*l0.b));
					if (is_certain(smaller))
					{
						// Reflex bisector?
						if (smaller)
							rResult = certified_is_smaller(sd_p_l0, sd_p_l1) ? ON_NEGATIVE_SIDE : ON_POSITIVE_SIDE;
						else
							rResult = certified_is_larger(sd_p_l0, sd_p_l1) ? ON_NEGATIVE_SIDE : ON_POSITIVE_SIDE;
					}
				}
			}
		}
	}
	catch (...)
	{
	}
	return rResult;
}
Uncertain<Sign> compare_offset_lines_isec_timesC2(Trisegment* const& m, Trisegment* const& n)
{
	Uncertain<Sign> rResult = Uncertain<Sign>::indeterminate();
	std::optional<Rational> mt_ = compute_offset_lines_isec_timeC2(m);
	std::optional<Rational> nt_ = compute_offset_lines_isec_timeC2(n);
	if (mt_ && nt_)
	{
		if (certified_is_positive(mt_) && certified_is_positive(nt_))
			rResult = certified_compare(mt_, nt_);
	}
	return rResult;
}
Uncertain<bool> are_events_simultaneousC2(Trisegment* const& l, Trisegment* const& r)
{
	Uncertain<bool> rResult = Uncertain<bool>::indeterminate();
	std::optional<Rational> lt_ = compute_offset_lines_isec_timeC2(l);
	std::optional<Rational> rt_ = compute_offset_lines_isec_timeC2(r);
	if (lt_ && rt_)
	{
		if (certified_is_positive(lt_) && certified_is_positive(rt_))
		{
			Uncertain<bool> equal_times = certified_is_equal(lt_, rt_);
			if (is_certain(equal_times))
			{
				if (equal_times)
				{
					std::optional<Point2D> li = construct_offset_lines_isecC2(l);
					std::optional<Point2D> ri = construct_offset_lines_isecC2(r);
					if (li && ri)
						rResult = certified_is_equal(li->x, ri->x) && certified_is_equal(li->y, ri->y);
				}
				else rResult = false;
			}
		}
	}
	return rResult;
}
Sign orientation(const Point2D &p, const Point2D &q, const Point2D &r)
{
	double px = p.x, py = p.y, qx = q.x, qy = q.y, rx = r.x, ry = r.y;
	double pqx = qx - px;
	double pqy = qy - py;
	double prx = rx - px;
	double pry = ry - py;
	double det = determinant(pqx, pqy, prx, pry);
	// Then semi-static filter.
	double maxx = abs(pqx);
	double maxy = abs(pqy);
	double aprx = abs(prx);
	double apry = abs(pry);
	if (maxx < aprx) maxx = aprx;
	if (maxy < apry) maxy = apry;
	// Sort them
	if (maxx > maxy)  std::swap(maxx, maxy);
	// Protect against underflow in the computation of eps.
	if (maxx < 1e-146) /* sqrt(min_double/eps) */
	{
		if (maxx == 0) return ZERO;
	}
	// Protect against overflow in the computation of det.
	else if (maxy < 1e153) /* sqrt(max_double [hadamard]/2) */
	{
		double eps = 8.8872057372592798e-16 * maxx * maxy;
		if (det > eps)  return POSITIVE;
		if (det < -eps) return NEGATIVE;
	}
	throw 0;
}
Sign orientation(const Point2D &p, const Point2D &q)
{
	return sign_of_determinant(p.x, p.y, q.x, q.y);
}

inline bool is_possibly_inexact_distance_clearly_not_zero(double n)
{
	return abs(n) > 1e-5;
}
inline bool is_possibly_inexact_time_clearly_not_zero(double const& n)
{
	return is_possibly_inexact_distance_clearly_not_zero(n);
}
inline bool is_possibly_inexact_distance_clearly_not_equal_to(double const& n, double const& m)
{
	return is_possibly_inexact_distance_clearly_not_zero(n - m);
}
Rational squared_distance_from_point_to_lineC2(double const& px, double const& py, double const& sx, double const& sy, double const& tx, double const& ty)
{
	double ldx = tx - sx;
	double ldy = ty - sy;
	double rdx = sx - px;
	double rdy = sy - py;
	double n = square(ldx * rdy - rdx * ldy);
	double d = square(ldx) + square(ldy);
	return Rational(n, d);
}
bool is_point_calculation_clearly_wrong(double const& t, Point2D const& p, Trisegment* const& aTrisegment)
{
	bool rR = false;
	if (is_possibly_inexact_time_clearly_not_zero(t))
	{
		Segment const& e0 = aTrisegment->e0();
		Segment const& e1 = aTrisegment->e1();
		Segment const& e2 = aTrisegment->e2();
		Point2D const& e0s = e0.start;
		Point2D const& e0t = e0.end;
		Point2D const& e1s = e1.start;
		Point2D const& e1t = e1.end;
		Point2D const& e2s = e2.start;
		Point2D const& e2t = e2.end;
		double const very_short(0.1);
		double const very_short_squared = square(very_short);
		double l0 = squared_distance(e0s, e0t);
		double l1 = squared_distance(e1s, e1t);
		double l2 = squared_distance(e2s, e2t);
		bool e0_is_not_very_short = l0 > very_short_squared;
		bool e1_is_not_very_short = l1 > very_short_squared;
		bool e2_is_not_very_short = l2 > very_short_squared;
		double d0 = squared_distance_from_point_to_lineC2(p.x, p.y, e0s.x, e0s.y, e0t.x, e0t.y).to_nt();
		double d1 = squared_distance_from_point_to_lineC2(p.x, p.y, e1s.x, e1s.y, e1t.x, e1t.y).to_nt();
		double d2 = squared_distance_from_point_to_lineC2(p.x, p.y, e2s.x, e2s.y, e2t.x, e2t.y).to_nt();
		double tt = square(t);
		bool e0_is_clearly_wrong = e0_is_not_very_short && is_possibly_inexact_distance_clearly_not_equal_to(d0, tt);
		bool e1_is_clearly_wrong = e1_is_not_very_short && is_possibly_inexact_distance_clearly_not_equal_to(d1, tt);
		bool e2_is_clearly_wrong = e2_is_not_very_short && is_possibly_inexact_distance_clearly_not_equal_to(d2, tt);
		rR = e0_is_clearly_wrong || e1_is_clearly_wrong || e2_is_clearly_wrong;
	}
	return rR;
}
std::optional<std::pair<double, Point2D>> Construct_ss_event_time_and_point_2(Trisegment* const& aTrisegment)
{
	bool lOK = false;
	double t(0);
	Point2D i(0, 0);
	std::optional<Rational> ot = compute_offset_lines_isec_timeC2(aTrisegment);
	if (!!ot && certainly(certified_is_not_zero(ot->d())))
	{
		t = ot->n() / ot->d();
		std::optional<Point2D> oi = construct_offset_lines_isecC2(aTrisegment);
		if (oi)
		{
			i = oi;
			lOK = true;
		}
	}
	return lOK ? std::optional<std::pair<double, Point2D>>(std::pair<double, Point2D>(t, i)) : std::optional<std::pair<double, Point2D>>();
}

#pragma float_control(pop)

inline bool HalfEdge::is_inner_bisector() const { return !this->vertex()->is_contour() && !this->opposite()->vertex()->is_contour(); }
inline bool HalfEdge::has_null_segment() const { return this->vertex()->has_null_point(); }
inline bool HalfEdge::has_infinite_time() const { return this->vertex()->has_infinite_time(); }


bool SSkeleton::is_valid()
{
	bool valid = (1 != (this->size_of_halfedges() & 1));
	// All halfedges.
	HalfEdgeIterator begin = this->halfedges_begin();
	HalfEdgeIterator end = this->halfedges_end();
	size_t n = 0;
	size_t nb = 0;
	for (; valid && (begin != end); begin++)
	{
		// Pointer integrity.
		valid = valid && ((*begin)->next() != NULL);
		if (!valid)
		{
			break;
		}
		valid = valid && ((*begin)->opposite() != NULL);
		if (!valid)
		{
			break;
		}
		// opposite integrity.
		valid = valid && ((*begin)->opposite() != *begin);
		if (!valid)
		{
			break;
		}
		valid = valid && ((*begin)->opposite()->opposite() == *begin);
		if (!valid)
		{
			break;
		}
		// previous integrity.
		valid = valid && (*begin)->next()->prev() == *begin;
		if (!valid)
		{
			break;
		}
		// vertex integrity.
		valid = valid && (*begin)->vertex() != NULL;
		if (!valid)
		{
			break;
		}
		if (!(*begin)->vertex()->has_infinite_time())
		{
			valid = valid && ((*begin)->vertex() == (*begin)->next()->opposite()->vertex());
			if (!valid)
			{
				break;
			}
		}
		// face integrity.
		valid = valid && ((*begin)->is_border() || (*begin)->face() != NULL);
		if (!valid)
		{
			break;
		}
		valid = valid && ((*begin)->face() == (*begin)->next()->face());
		if (!valid)
		{
			break;
		}
		++n;
		if ((*begin)->is_border())
			++nb;
	}
	bool nvalid = (n == this->size_of_halfedges());
	valid = valid && nvalid;
	// All vertices.
	VertexIterator vbegin = this->vertices_begin();
	VertexIterator vend = this->vertices_end();
	size_t v = 0;
	n = 0;
	bool is_partial_skeleton = false;

	for (; valid && (vbegin != vend); ++vbegin)
	{
		// Pointer integrity.
		valid = valid && (*vbegin)->halfedge() != NULL;
		if (!valid)
		{
			break;
		}

		// cycle-around-vertex test.
		if (!(*vbegin)->has_infinite_time())
		{
			valid = valid && (*vbegin)->halfedge()->vertex() == *vbegin;
			if (!valid)
			{
				break;
			}
			HalfEdge* h = (*vbegin)->halfedge();
			if (h != NULL)
			{
				HalfEdge* g = h;
				do
				{
					++n;
					h = h->next()->opposite();
					valid = valid && (n <= this->size_of_halfedges() && n != 0);
				} while (valid && (h != g));
			}
		}
		else is_partial_skeleton = true;

		++v;
	}

	if (!is_partial_skeleton)
	{
		bool vvalid = (v == this->size_of_vertices());
		bool vnvalid = n == this->size_of_halfedges();
		valid = valid && vvalid && vnvalid;
	}

	// All faces.
	FaceIterator fbegin = this->faces_begin();
	FaceIterator fend = this->faces_end();
	size_t f = 0;
	n = 0;
	for (; valid && (fbegin != fend); ++fbegin)
	{
		valid = valid && (*fbegin)->halfedge() != NULL;
		if (!valid)
		{
			break;
		}

		valid = valid && (*fbegin)->halfedge()->face() == *fbegin;
		if (!valid)
		{
			break;
		}
		// cycle-around-face test.
		HalfEdge* h = (*fbegin)->halfedge();
		if (h != NULL)
		{
			HalfEdge* g = h;
			do
			{
				++n;
				h = h->next();
				valid = valid && (n <= this->size_of_halfedges() && n != 0);
			} while (valid && (h != g));
		}
		++f;
	}
	bool fvalid = (f == this->size_of_faces());
	bool fnvalid = (n + nb == this->size_of_halfedges());
	valid = valid && fvalid && fnvalid;
	return valid;
}
