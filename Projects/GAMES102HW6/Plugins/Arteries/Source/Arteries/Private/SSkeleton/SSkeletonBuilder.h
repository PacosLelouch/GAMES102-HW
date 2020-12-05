// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "SSkeleton.h"
/*
inline Uncertain<bool> logical_or(Uncertain<bool> a, Uncertain<bool> b) { return a | b; }
inline Uncertain<bool> logical_and(Uncertain<bool> a, Uncertain<bool> b) { return a & b; }
inline Uncertain<bool> logical_or(Uncertain<bool> a, Uncertain<bool> b, Uncertain<bool> c) { return a | b | c; }
inline Uncertain<bool> logical_and(Uncertain<bool> a, Uncertain<bool> b, Uncertain<bool> c) { return a & b & c; }
*/

class SSkeletonBuilder
{
public:
	enum Site { AT_SOURCE = -1, INSIDE = 0, AT_TARGET = +1 };
	struct Multinode
	{
		Multinode(HalfEdge* b, HalfEdge* e) :begin(b), end(e), v(b->vertex()), size(0) {}
		HalfEdge* begin;
		HalfEdge* end;
		Vertex* v;
		size_t size;
		std::vector<HalfEdge*> bisectors_to_relink;
		std::vector<HalfEdge*> bisectors_to_remove;
		std::vector<Vertex*> nodes_to_remove;
	};
	struct MultinodeComparer
	{
		bool operator() (Multinode* const& x, Multinode* const& y) { return x->size > y->size; }
	};
	struct Halfedge_ID_compare
	{
		bool operator() (HalfEdge* const& aA, HalfEdge* const& aB) const
		{
			return aA->id() < aB->id();
		}
	};
public:
	SSkeletonBuilder(std::optional<double> aMaxTime = std::optional<double>());
	~SSkeletonBuilder();
	SSkeleton* construct_skeleton(bool aNull_if_failed = true);
	class Event_compare
	{
	public:
		Event_compare(SSkeletonBuilder const* aBuilder) : mBuilder(aBuilder) {}
		bool operator() (Event* const& aA, Event* const& aB) const
		{
			return mBuilder->CompareEvents(aA, aB) == LARGER;
		}
	private:
		SSkeletonBuilder const* mBuilder;
	};
	typedef std::priority_queue<Event*, std::vector<Event*>, Event_compare> PQ;
	struct Vertex_data
	{
		Vertex_data(Vertex* aVertex, Event_compare const& aComparer) :mVertex(aVertex), mIsReflex(false), mIsDegenerate(false), mIsProcessed(false), mIsExcluded(false), mPrevInLAV(-1), mNextInLAV(-1), mNextSplitEventInMainPQ(false), mSplitEvents(aComparer), mTrisegment(NULL) {}
		Vertex* mVertex;
		bool mIsReflex;
		bool mIsDegenerate;
		bool mIsProcessed;
		bool mIsExcluded;
		int mPrevInLAV;
		int mNextInLAV;
		bool mNextSplitEventInMainPQ;
		PQ mSplitEvents;
		Triedge mTriedge; // Here, E0,E1 corresponds to the vertex (unlike *event* triedges)
		Trisegment* mTrisegment; // Skeleton nodes cache the full trisegment tree that defines the originating event
	};
private:
	HalfEdge* validate(HalfEdge* aH) const;
	Vertex* validate(Vertex*   aH) const;
	void InitVertexData(Vertex* aV)
	{
		mVertexData.push_back(GCNEW(Vertex_data(aV, mEventCompare)));
	}
	Vertex_data const& GetVertexData(Vertex* aV) const
	{
		return *mVertexData[aV->id()];
	}
	Vertex_data&  GetVertexData(Vertex* aV)
	{
		return *mVertexData[aV->id()];
	}
	Vertex* GetVertex(int aIdx)
	{
		return mVertexData[aIdx]->mVertex;
	}
	Trisegment* const& GetTrisegment(Vertex* aV) const
	{
		return GetVertexData(aV).mTrisegment;
	}
	void SetTrisegment(Vertex* aV, Trisegment* const& aTrisegment)
	{
		GetVertexData(aV).mTrisegment = aTrisegment;
	}
	Triedge const& GetVertexTriedge(Vertex* aV) const
	{
		return GetVertexData(aV).mTriedge;
	}
	void SetVertexTriedge(Vertex* aV, Triedge const& aTriedge)
	{
		GetVertexData(aV).mTriedge = aTriedge;
	}
	Segment CreateSegment(HalfEdge* aH) const
	{
		Point2D s = aH->opposite()->vertex()->point();
		Point2D t = aH->vertex()->point();
		return Segment(s, t);
	}
	Point2D CreateVector(HalfEdge* aH) const
	{
		Point2D s = aH->opposite()->vertex()->point();
		Point2D t = aH->vertex()->point();
		return t - s;
	}
	Point2D CreateDirection(HalfEdge* aH) const
	{
		return CreateVector(aH);
	}
	Trisegment* CreateTrisegment(Triedge const& aTriedge) const
	{
		Segment e0 = CreateSegment(aTriedge.e0());
		Segment e1 = CreateSegment(aTriedge.e1());
		Segment e2 = CreateSegment(aTriedge.e2());
		Uncertain<Trisegment_collinearity> lCollinearity = certified_trisegment_collinearity(e0, e1, e2);
		if (is_certain(lCollinearity)) return GCNEW(Trisegment(e0, e1, e2, lCollinearity));
		return NULL;
	}
	Trisegment* CreateTrisegment(Triedge const& aTriedge, Vertex* aLSeed) const
	{
		Trisegment* r = CreateTrisegment(aTriedge);
		r->set_child_l(GetTrisegment(aLSeed));
		return r;
	}
	Trisegment* CreateTrisegment(Triedge const& aTriedge, Vertex* aLSeed, Vertex* aRSeed) const
	{
		Trisegment* r = CreateTrisegment(aTriedge);
		r->set_child_l(GetTrisegment(aLSeed));
		r->set_child_r(GetTrisegment(aRSeed));
		return r;
	}
	Vertex* GetPrevInLAV(Vertex* aV)
	{
		return GetVertex(GetVertexData(aV).mPrevInLAV);
	}
	Vertex* GetNextInLAV(Vertex* aV)
	{
		return GetVertex(GetVertexData(aV).mNextInLAV);
	}
	void SetPrevInLAV(Vertex* aV, Vertex* aPrev)
	{
		GetVertexData(aV).mPrevInLAV = aPrev->id();
	}
	void SetNextInLAV(Vertex* aV, Vertex* aPrev)
	{
		GetVertexData(aV).mNextInLAV = aPrev->id();
	}
	HalfEdge* GetEdgeEndingAt(Vertex* aV)
	{
		return GetVertexTriedge(aV).e0();
	}
	HalfEdge* GetEdgeStartingAt(Vertex* aV)
	{
		return GetEdgeEndingAt(GetNextInLAV(aV));
	}
	void Exclude(Vertex* aV)
	{
		GetVertexData(aV).mIsExcluded = true;
	}
	bool IsExcluded(Vertex* aV) const
	{
		return GetVertexData(aV).mIsExcluded;
	}
	void SetIsReflex(Vertex* aV)
	{
		GetVertexData(aV).mIsReflex = true;
	}
	bool IsReflex(Vertex* aV)
	{
		return GetVertexData(aV).mIsReflex;
	}
	void SetIsDegenerate(Vertex* aV)
	{
		GetVertexData(aV).mIsDegenerate = true;
	}
	bool IsDegenerate(Vertex* aV)
	{
		return GetVertexData(aV).mIsDegenerate;
	}
	void SetIsProcessed(Vertex* aV)
	{
		GetVertexData(aV).mIsProcessed = true;
	}
	bool IsConvex(Vertex* aV)
	{
		Vertex_data const& lData = GetVertexData(aV);
		return !lData.mIsReflex && !lData.mIsDegenerate;
	}
	bool IsProcessed(Vertex* aV)
	{
		return GetVertexData(aV).mIsProcessed;
	}
	void AddSplitEvent(Vertex* aV, Event* aEvent)
	{
		GetVertexData(aV).mSplitEvents.push(aEvent);
	}
	Event* PopNextSplitEvent(Vertex* aV)
	{
		Event* rEvent = NULL;
		Vertex_data& lData = GetVertexData(aV);
		if (!lData.mNextSplitEventInMainPQ)
		{
			PQ& lPQ = lData.mSplitEvents;
			if (!lPQ.empty())
			{
				rEvent = lPQ.top();
				lPQ.pop();
				lData.mNextSplitEventInMainPQ = true;
			}
		}
		return rEvent;
	}
	void AllowNextSplitEvent(Vertex* aV)
	{
		GetVertexData(aV).mNextSplitEventInMainPQ = false;
	}
	void InsertEventInPQ(Event* aEvent);
	Event* PopEventFromPQ();
	bool ExistEvent(Trisegment* const& aS)
	{
		return Do_ss_event_exist_2(aS, mMaxTime);
	}
	bool IsOppositeEdgeFacingTheSplitSeed(Vertex* aSeed, HalfEdge* aOpposite) const
	{
		if (aSeed->is_skeleton())
			return Is_edge_facing_ss_node_2(GetTrisegment(aSeed), CreateSegment(aOpposite));
		else
			return Is_edge_facing_ss_node_2(aSeed->point(), CreateSegment(aOpposite));
	}
	Sign EventPointOrientedSide(Event const& aEvent, HalfEdge* aE0, HalfEdge* aE1, Vertex* aV01, bool aE0isPrimary) const
	{
		return oriented_side_of_event_point_wrt_bisectorC2(aEvent.trisegment(), CreateSegment(aE0), CreateSegment(aE1), GetTrisegment(aV01), aE0isPrimary);
	}
	Sign CompareEvents(Trisegment* const& aA, Trisegment* const& aB) const
	{
		return compare_offset_lines_isec_timesC2(aA, aB);
	}
	Sign CompareEvents(Event* const& aA, Event* const& aB) const
	{
		return aA->triedge() != aB->triedge() ? CompareEvents(aA->trisegment(), aB->trisegment()) : EQUAL;
	}
	bool AreEventsSimultaneous(Trisegment* const& x, Trisegment* const& y) const
	{
		return are_events_simultaneousC2(x, y);
	}
	bool AreEventsSimultaneous(Event* const& x, Event* const& y) const
	{
		return AreEventsSimultaneous(x->trisegment(), y->trisegment());
	}
	bool AreContourNodesCoincident(Vertex* aX, Vertex* aY) const
	{
		return compare_xy(aX->point(), aY->point()) == EQUAL;
	}
	bool AreSkeletonNodesCoincident(Vertex* aX, Vertex* aY) const
	{
		return AreEventsSimultaneous(GetTrisegment(aX), GetTrisegment(aY));
	}
	Sign CompareEvents(Trisegment* const& aTrisegment, Vertex* aSeedNode) const
	{
		return aSeedNode->is_skeleton() ? aSeedNode->has_infinite_time() ? SMALLER : CompareEvents(aTrisegment, GetTrisegment(aSeedNode)) : LARGER;
	}
	void SetBisectorSlope(HalfEdge* aBisector, Sign aSlope)
	{
		aBisector->set_slope(aSlope);
	}
	void SetBisectorSlope(Vertex* aA, Vertex* aB)
	{
		HalfEdge* lOBisector = aA->primary_bisector();
		HalfEdge* lIBisector = lOBisector->opposite();
		if (aA->is_contour())
		{
			SetBisectorSlope(lOBisector, POSITIVE);
			SetBisectorSlope(lIBisector, NEGATIVE);
		}
		else if (aB->is_contour())
		{
			SetBisectorSlope(lOBisector, NEGATIVE);
			SetBisectorSlope(lIBisector, POSITIVE);
		}
		else
		{
			if (aA->has_infinite_time())
			{
				SetBisectorSlope(lOBisector, NEGATIVE);
				SetBisectorSlope(lIBisector, POSITIVE);
			}
			else if (aB->has_infinite_time())
			{
				SetBisectorSlope(lOBisector, NEGATIVE);
				SetBisectorSlope(lIBisector, POSITIVE);
			}
			else
			{
				Sign lSlope = CompareEvents(GetTrisegment(aB), GetTrisegment(aA));
				SetBisectorSlope(lOBisector, lSlope);
				SetBisectorSlope(lIBisector,-lSlope);
			}
		}
	}
	std::pair<double, Point2D> ConstructEventTimeAndPoint(Trisegment* const& aS) const
	{
		std::pair<double, Point2D> r = Construct_ss_event_time_and_point_2(aS);
		return r;
	}
	void SetEventTimeAndPoint(Event& aE)
	{
		std::pair<double, Point2D> r = ConstructEventTimeAndPoint(aE.trisegment());
		aE.SetTimeAndPoint(r.first, r.second);
	}
	void EraseBisector(HalfEdge* aB)
	{
		mSSkel->edges_erase(aB);
	}
	void Link(HalfEdge* aH, Face* aF)
	{
		aH->set_face(aF);
	}
	void Link(HalfEdge* aH, Vertex* aV)
	{
		aH->set_vertex(aV);
	}
	void Link(Vertex* aV, HalfEdge* aH)
	{
		aV->set_halfedge(aH);
	}
	void CrossLinkFwd(HalfEdge* aPrev, HalfEdge* aNext)
	{
		aPrev->set_next(aNext);
		aNext->set_prev(aPrev);
	}
	void CrossLink(HalfEdge* aH, Vertex* aV)
	{
		Link(aH, aV);
		Link(aV, aH);
	}
	Triedge GetCommonTriedge(Vertex* aA, Vertex* aB);
	bool AreBisectorsCoincident(HalfEdge* aA, HalfEdge* aB) const;
	Event* IsPseudoSplitEvent(Event* const& aEvent, VertexPair aOpp, Site const& aSite);
	void CollectSplitEvent(Vertex* aNode, Triedge const& aTriedge);
	void CollectSplitEvents(Vertex* aNode, Triedge const& aPrevEventTriedge);
	Event* FindEdgeEvent(Vertex* aLNode, Vertex* aRNode, Triedge const& aPrevEventTriedge);
	void HandleSimultaneousEdgeEvent(Vertex* aA, Vertex* aB);
	void CollectNewEvents(Vertex* aNode, Triedge const& aPrevEventTriedge);
	void UpdatePQ(Vertex* aV, Triedge const& aPrevEventTriedge);
	void CreateInitialEvents();
	void CreateContourBisectors();
	void InitPhase();
	void SetupNewNode(Vertex* aNode);
	VertexPair LookupOnSLAV(HalfEdge* aOBorder, Event* const& aEvent, Site& rSite);
	Vertex* ConstructEdgeEventNode(EdgeEvent&   aEvent);
	VertexPair ConstructSplitEventNodes(SplitEvent&  aEvent, Vertex* aOppR);
	VertexPair ConstructPseudoSplitEventNodes(PseudoSplitEvent& aEvent);
	bool IsValidEdgeEvent(EdgeEvent const& aEvent);
	bool IsValidSplitEvent(SplitEvent const& aEvent);
	bool IsValidPseudoSplitEvent(PseudoSplitEvent const& aEvent);
	void HandleEdgeEvent(Event* aEvent);
	void HandleSplitEvent(Event* aEvent, VertexPair aOpp);
	void HandlePseudoSplitEvent(Event* aEvent);
	void HandleSplitOrPseudoSplitEvent(Event* aEvent);
	void InsertNextSplitEventInPQ(Vertex* v);
	void InsertNextSplitEventsInPQ();
	bool IsProcessed(Event* aEvent);
	void Propagate();
	void EraseNode(Vertex* aNode);
	void MergeSplitNodes(VertexPair aSplitNodes);
	void RelinkBisectorsAroundMultinode(Vertex* const& v0, std::vector<HalfEdge*>& aLinks);
	void PreprocessMultinode(Multinode& aMN);
	void ProcessMultinode(Multinode& aMN, std::vector<HalfEdge*>& rHalfedgesToRemove, std::vector<Vertex*>& rNodesToRemove);
	Multinode* CreateMultinode(HalfEdge* begin, HalfEdge* end);
	void MergeCoincidentNodes();
	bool FinishUp();
	bool Run();
private:
	std::vector<Vertex_data*> mVertexData;
	std::vector<Vertex*> mReflexVertices;
	std::vector<HalfEdge*> mDanglingBisectors;
	std::vector<HalfEdge*> mContourHalfedges;
	std::list<Vertex*> mGLAV;
	std::vector<VertexPair> mSplitNodes;
	Event_compare mEventCompare;
	int mVertexID;
	int mEdgeID;
	int mFaceID;
	int mEventID;
	int mStepID;
	std::optional<double> mMaxTime;
	PQ mPQ;
	SSkeleton* mSSkel;
	GCollector GC;
private:
	void enter_valid_contour(std::vector<Point2D>& polygon)
	{
		HalfEdge* lFirstCCWBorder = NULL;
		HalfEdge* lPrevCCWBorder = NULL;
		HalfEdge* lNextCWBorder = NULL;
		Vertex* lFirstVertex = NULL;
		Vertex* lPrevVertex = NULL;
		std::vector<Point2D>::iterator aBegin = polygon.begin();
		std::vector<Point2D>::iterator aEnd = polygon.end();
		std::vector<Point2D>::iterator lCurr = aBegin;
		int c = 0;
		while (lCurr != aEnd)
		{
			HalfEdge* lCCWBorder = mSSkel->edges_push_back(GCNEW(HalfEdge(mEdgeID)), GCNEW(HalfEdge(mEdgeID + 1)));
			HalfEdge* lCWBorder = lCCWBorder->opposite();
			mEdgeID += 2;
			mContourHalfedges.push_back(lCCWBorder);
			Vertex* lVertex = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++, *lCurr)));
			InitVertexData(lVertex);
			Face* lFace = mSSkel->faces_push_back(GCNEW(Face(mFaceID++)));
			++c;
			lCCWBorder->set_face(lFace);
			lFace->set_halfedge(lCCWBorder);
			lVertex->set_halfedge(lCCWBorder);
			lCCWBorder->set_vertex(lVertex);
			if (lCurr == aBegin)
			{
				lFirstVertex = lVertex;
				lFirstCCWBorder = lCCWBorder;
			}
			else
			{
				SetPrevInLAV(lVertex, lPrevVertex);
				SetNextInLAV(lPrevVertex, lVertex);
				SetVertexTriedge(lPrevVertex, Triedge(lPrevCCWBorder, lCCWBorder));
				lCWBorder->set_vertex(lPrevVertex);
				lCCWBorder->set_prev(lPrevCCWBorder);
				lPrevCCWBorder->set_next(lCCWBorder);
				lNextCWBorder->set_prev(lCWBorder);
				lCWBorder->set_next(lNextCWBorder);
			}
			++lCurr;
			lPrevVertex = lVertex;
			lPrevCCWBorder = lCCWBorder;
			lNextCWBorder = lCWBorder;
		}
		SetPrevInLAV(lFirstVertex, lPrevVertex);
		SetNextInLAV(lPrevVertex, lFirstVertex);
		SetVertexTriedge(lPrevVertex, Triedge(lPrevCCWBorder, lFirstCCWBorder));
		lFirstCCWBorder->opposite()->set_vertex(lPrevVertex);
		lFirstCCWBorder->set_prev(lPrevCCWBorder);
		lPrevCCWBorder->set_next(lFirstCCWBorder);
		lPrevCCWBorder->opposite()->set_prev(lFirstCCWBorder->opposite());
		lFirstCCWBorder->opposite()->set_next(lPrevCCWBorder->opposite());
	}
public:
	void enter_contour(std::vector<Point2D>& polygon)
	{
		enter_valid_contour(polygon);
	}
};

DECLARE_PARSE_TYPE(Vertex);
DECLARE_PARSE_TYPE(HalfEdge);
DECLARE_PARSE_TYPE(Face);
DECLARE_PARSE_TYPE(Trisegment);
DECLARE_PARSE_TYPE(SSkeleton);
DECLARE_PARSE_TYPE(SplitEvent);
DECLARE_PARSE_TYPE(EdgeEvent);
DECLARE_PARSE_TYPE(PseudoSplitEvent);
DECLARE_PARSE_TYPE(SSkeletonBuilder::Multinode);
DECLARE_PARSE_TYPE(SSkeletonBuilder::Vertex_data);