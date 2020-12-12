// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "SSkeletonBuilder.h"

IMPLEMENT_PARSE_TYPE(Vertex);
IMPLEMENT_PARSE_TYPE(HalfEdge);
IMPLEMENT_PARSE_TYPE(Face);
IMPLEMENT_PARSE_TYPE(Trisegment);
IMPLEMENT_PARSE_TYPE(SSkeleton);
IMPLEMENT_PARSE_TYPE(SplitEvent);
IMPLEMENT_PARSE_TYPE(EdgeEvent);
IMPLEMENT_PARSE_TYPE(PseudoSplitEvent);
IMPLEMENT_PARSE_TYPE(SSkeletonBuilder::Multinode);
IMPLEMENT_PARSE_TYPE(SSkeletonBuilder::Vertex_data);

SSkeletonBuilder::SSkeletonBuilder(std::optional<double> aMaxTime)
	:
	mEventCompare(this)
	, mVertexID(0)
	, mEdgeID(0)
	, mFaceID(0)
	, mEventID(0)
	, mStepID(0)
	, mMaxTime(aMaxTime)
	, mPQ(mEventCompare)
{
	mSSkel = GCNEW(SSkeleton());
}
SSkeletonBuilder::~SSkeletonBuilder()
{
	GC.Collect();
}
bool SSkeletonBuilder::Run()
{
	InitPhase();
	Propagate();
	return FinishUp();
}
void SSkeletonBuilder::InitPhase()
{
	CreateContourBisectors();
	CreateInitialEvents();
}
bool SSkeletonBuilder::FinishUp()
{
	for (auto it = mSplitNodes.begin(); it != mSplitNodes.end(); ++it)
		MergeSplitNodes(*it);
	for (auto it = mDanglingBisectors.begin(); it != mDanglingBisectors.end(); ++it)
		EraseBisector(*it);
	MergeCoincidentNodes();
	return mSSkel->is_valid();
}
HalfEdge* SSkeletonBuilder::validate(HalfEdge* aH) const
{
	if (!aH) throw std::runtime_error("Incomplete straight skeleton");
	return aH;
}
Vertex* SSkeletonBuilder::validate(Vertex* aH) const
{
	if (!aH) throw std::runtime_error("Incomplete straight skeleton");
	return aH;
}
SSkeletonBuilder::Multinode* SSkeletonBuilder::CreateMultinode(HalfEdge* begin, HalfEdge* end)
{
	return GCNEW(Multinode(begin, end));
}
void SSkeletonBuilder::PreprocessMultinode(Multinode& aMN)
{
	//
	// A Multinode is a run of coincident nodes along a face.
	// Its represented by a pair of halfedges describing a linear profile.
	// The first halfedge in the pair points to the first node in the multinode.
	// Each ->next() halfedge in the profile points to a subsequent node.
	// The second halfedge in the pair is past-the-end (it points to the first node around the face that IS NOT part of the multinode)
	//
	// Halfedge_handle oend = validate(aMN.end->opposite());
	HalfEdge* h = aMN.begin;
	aMN.bisectors_to_relink.push_back(h);
	// Traverse the profile collecting:
	//  The nodes to be removed from the HDS (all but the first)
	//  The bisectors to be removed from the HDS (each bisector pointing to the next node in the multinode)
	//  The bisectors around each node that must be relinked to the first node (which will be kept in place of the multinode)
	do
	{
		++aMN.size;
		HalfEdge* nx = validate(h->next());
		if (nx != aMN.end)
			aMN.bisectors_to_remove.push_back(nx);
		// Since each halfedge "h" in this lineal profile corresponds to a single face, all the bisectors around
		// each node which must be relinked are those found ccw between h and h->next()
		HalfEdge* ccw = h;
		HalfEdge* ccw_end = validate(h->next()->opposite());
		for (;;)
		{
			ccw = validate(ccw->opposite()->prev());
			if (ccw != ccw_end)
				aMN.bisectors_to_relink.push_back(ccw);
			else break;
		}
		if (h != aMN.begin)
		{
			aMN.nodes_to_remove.push_back(h->vertex());
		}
		h = nx;
	} while (h != aMN.end);
	aMN.bisectors_to_relink.push_back(aMN.end->opposite());
}
void SSkeletonBuilder::ProcessMultinode(Multinode& aMN, std::vector<HalfEdge*>& rBisectorsToRemove, std::vector<Vertex*>& rNodesToRemove)
{
	bool lDoNotProcess = false;
	HalfEdge* h = aMN.begin;
	do
	{
		if (h->vertex()->has_infinite_time() || IsExcluded(h->vertex()))
			lDoNotProcess = true;
	} while (h = h->next(), !lDoNotProcess && h != aMN.end);
	if (!lDoNotProcess)
	{
		h = aMN.begin;
		do
		{
			Exclude(h->vertex());
		} while (h = h->next(), h != aMN.end);
		std::copy(aMN.bisectors_to_remove.begin(), aMN.bisectors_to_remove.end(), std::back_inserter(rBisectorsToRemove));
		for (std::vector<Vertex*>::iterator vi = aMN.nodes_to_remove.begin(), evi = aMN.nodes_to_remove.end(); vi != evi; ++vi)
			rNodesToRemove.push_back(*vi);
		RelinkBisectorsAroundMultinode(aMN.v, aMN.bisectors_to_relink);
	}
}
void SSkeletonBuilder::RelinkBisectorsAroundMultinode(Vertex* const& v0, std::vector<HalfEdge*>& aLinks)
{
	// Connect the bisectors with each other following the CCW ordering
	HalfEdge* first_he = aLinks.front();
	HalfEdge* prev_he = first_he;
	first_he->set_vertex(v0);
	for (std::vector<HalfEdge*>::iterator i = /*cpp11::next*/std::next(aLinks.begin()), ei = aLinks.end(); i != ei; ++i)
	{
		HalfEdge* he = *i;
		he->set_vertex(v0);
		HalfEdge* prev_he_opp = prev_he->opposite();
		he->set_next(prev_he_opp);
		prev_he_opp->set_prev(he);
		prev_he = he;
	}
	HalfEdge* prev_he_opp = prev_he->opposite();
	first_he->set_next(prev_he_opp);
	prev_he_opp->set_prev(first_he);
	// Reset the main halfedge for v0  
	v0->set_halfedge(first_he);
}
void SSkeletonBuilder::MergeSplitNodes(VertexPair aSplitNodes)
{
	Vertex* lLNode = aSplitNodes.first, *lRNode = aSplitNodes.second;
	HalfEdge* lIBisectorL1 = lLNode->primary_bisector()->opposite();
	HalfEdge* lIBisectorR1 = lRNode->primary_bisector()->opposite();
	HalfEdge* lIBisectorL2 = lIBisectorL1->next()->opposite();
	HalfEdge* lIBisectorR2 = lIBisectorR1->next()->opposite();
	if (lIBisectorL1->vertex() == lRNode)
		lIBisectorL1->set_vertex(lLNode);
	if (lIBisectorR1->vertex() == lRNode)
		lIBisectorR1->set_vertex(lLNode);
	if (lIBisectorL2->vertex() == lRNode)
		lIBisectorL2->set_vertex(lLNode);
	if (lIBisectorR2->vertex() == lRNode)
		lIBisectorR2->set_vertex(lLNode);
	EraseNode(lRNode);
}
void SSkeletonBuilder::MergeCoincidentNodes()
{
	std::vector<Multinode*> lMultinodes;
	for (FaceIterator it = mSSkel->faces_begin(); it != mSSkel->faces_end(); ++it)
	{
		Face* fit = *it;
		// 'h' is the first (CCW) skeleton halfedge.
		HalfEdge* h = validate(validate(fit->halfedge())->next());
		// 'last' is the last (CCW) skeleton halfedge
		HalfEdge* last = validate(fit->halfedge()->prev());
		HalfEdge* h0 = h;
		Vertex* v0 = validate(h0->vertex());
		if (!v0->has_infinite_time())
		{
			h = validate(h->next());
			while (h != last)
			{
				Vertex* v = validate(h->vertex());
				if (!v->has_infinite_time())
				{
					if (!AreSkeletonNodesCoincident(v0, v))
					{
						if (h0->next() != h)
							lMultinodes.push_back(CreateMultinode(h0, h));
						v0 = v;
						h0 = h;
					}
				}
				h = validate(h->next());
			}
			if (h0->next() != h)
				lMultinodes.push_back(CreateMultinode(h0, h));
		}
	}
	//
	// The merging loop removes all but one of the coincident skeleton nodes and the halfedges between them.
	// But it can't physically erase those from the HDS while looping, so the nodes/bisector to erase 
	// are collected in these sequences are erased after the merging loop.
	// 
	std::vector<HalfEdge*> lBisectorsToRemove;
	std::vector<Vertex*> lNodesToRemove;
	for (std::vector<Multinode*>::iterator it = lMultinodes.begin(), eit = lMultinodes.end(); it != eit; ++it)
		PreprocessMultinode(**it);
	std::sort(lMultinodes.begin(), lMultinodes.end(), MultinodeComparer());
	for (std::vector<Multinode*>::iterator it = lMultinodes.begin(), eit = lMultinodes.end(); it != eit; ++it)
		ProcessMultinode(**it, lBisectorsToRemove, lNodesToRemove);
	for (std::vector<HalfEdge*>::iterator hi = lBisectorsToRemove.begin(), ehi = lBisectorsToRemove.end(); hi != ehi; ++hi)
	{
		(*hi)->reset_id(-1);
		mSSkel->edges_erase(*hi);
	}
	for (std::vector<Vertex*>::iterator vi = lNodesToRemove.begin(), evi = lNodesToRemove.end(); vi != evi; ++vi)
		EraseNode(*vi);
}
void SSkeletonBuilder::CreateContourBisectors()
{
	for (VertexIterator it = mSSkel->vertices_begin(); it != mSSkel->vertices_end(); ++it)
	{
		Vertex* v = *it;
		mGLAV.push_back(v);
		Vertex* lPrev = GetPrevInLAV(v);
		Vertex* lNext = GetNextInLAV(v);
		Sign lOrientation = orientation(lPrev->point(), v->point(), lNext->point());
		if (lOrientation == COLLINEAR)
		{
			SetIsDegenerate(v);
		}
		else if (lOrientation == RIGHT_TURN)
		{
			mReflexVertices.push_back(v);
			SetIsReflex(v);
		}
		HalfEdge* lOB = GCNEW(HalfEdge(mEdgeID++));
		HalfEdge* lIB = GCNEW(HalfEdge(mEdgeID++));
		HalfEdge* lOBisector = mSSkel->edges_push_back(lOB, lIB);
		HalfEdge* lIBisector = lOBisector->opposite();
		lOBisector->set_face(v->halfedge()->face());
		lIBisector->set_face(v->halfedge()->next()->face());
		lIBisector->set_vertex(v);
		HalfEdge* lIBorder = v->halfedge();
		HalfEdge* lOBorder = v->halfedge()->next();
		lIBorder->set_next(lOBisector);
		lOBisector->set_prev(lIBorder);
		lOBorder->set_prev(lIBisector);
		lIBisector->set_next(lOBorder);
	}
	for (FaceIterator it = mSSkel->faces_begin(); it != mSSkel->faces_end(); ++it)
	{
		Face* fit = *it;
		HalfEdge* lBorder = fit->halfedge();
		HalfEdge* lLBisector = lBorder->prev();
		HalfEdge* lRBisector = lBorder->next();
		Vertex* lInfNode = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++)));
		InitVertexData(lInfNode);
		lRBisector->set_next(lLBisector);
		lLBisector->set_prev(lRBisector);
		lRBisector->set_vertex(lInfNode);
		lInfNode->set_halfedge(lRBisector);
		SetBisectorSlope(lRBisector, POSITIVE);
		SetBisectorSlope(lLBisector, NEGATIVE);
	}
}
void SSkeletonBuilder::CreateInitialEvents()
{
	Triedge const cNull_triedge;
	for (VertexIterator it = mSSkel->vertices_begin(); it != mSSkel->vertices_end(); ++it)
	{
		Vertex* v = *it;
		if (!v->has_infinite_time())
		{
			UpdatePQ(v, cNull_triedge);
		}
	}
}
void SSkeletonBuilder::UpdatePQ(Vertex* aNode, Triedge const& aPrevEventTriedge)
{
	Vertex* lPrev = GetPrevInLAV(aNode);
	Vertex* lNext = GetNextInLAV(aNode);
	HalfEdge* lOBisector_P = lPrev->primary_bisector();
	HalfEdge* lOBisector_C = aNode->primary_bisector();
	HalfEdge* lOBisector_N = lNext->primary_bisector();
	if (AreBisectorsCoincident(lOBisector_C, lOBisector_P))
		HandleSimultaneousEdgeEvent(aNode, lPrev);
	else if (AreBisectorsCoincident(lOBisector_C, lOBisector_N))
		HandleSimultaneousEdgeEvent(aNode, lNext);
	else
		CollectNewEvents(aNode, aPrevEventTriedge);
}
bool SSkeletonBuilder::AreBisectorsCoincident(HalfEdge* aA, HalfEdge* aB) const
{
	HalfEdge* lA_LBorder = aA->defining_contour_edge();
	HalfEdge* lA_RBorder = aA->opposite()->defining_contour_edge();
	HalfEdge* lB_LBorder = aB->defining_contour_edge();
	HalfEdge* lB_RBorder = aB->opposite()->defining_contour_edge();
	return (lA_LBorder == lB_LBorder && lA_RBorder == lB_RBorder) || (lA_LBorder == lB_RBorder && lA_RBorder == lB_LBorder);
}
void SSkeletonBuilder::HandleSimultaneousEdgeEvent(Vertex* aA, Vertex* aB)
{
	HalfEdge* lOA = aA->primary_bisector();
	HalfEdge* lOB = aB->primary_bisector();
	HalfEdge* lIA = lOA->opposite();
	HalfEdge* lIB = lOB->opposite();
	Vertex* lOAV = lOA->vertex();
	Vertex* lIAV = lIA->vertex();
	Vertex* lOBV = lOB->vertex();
// Vertex_handle lIBV = lIB->vertex() ;
	SetIsProcessed(aA);
	SetIsProcessed(aB);
	mGLAV.remove(aA);
	mGLAV.remove(aB);
	HalfEdge* lOA_Prev = lOA->prev();
	HalfEdge* lIA_Next = lIA->next();
	HalfEdge* lOB_Prev = lOB->prev();
	HalfEdge* lIB_Next = lIB->next();
	(void)lOB_Prev; // may be unused
	(void)lIB_Next; // may be unused
	CrossLinkFwd(lOB, lIA_Next);
	CrossLinkFwd(lOA_Prev, lIB);
	Link(lOB, aA);
	mDanglingBisectors.push_back(lOA);
	// The code above corrects the links for vertices aA/aB to the erased halfedges lOA and lIA.
	// However, any of these vertices (aA/aB) maybe one of the twin vertices of a split event.
	// If that's the case, the erased halfedge maybe be linked to a 'couple' of those vertices.
	// This situation is corrected below:
	if (!lOAV->has_infinite_time() && lOAV != aA && lOAV != aB)
	{
		Link(lOAV, lIB);
	}
	if (!lIAV->has_infinite_time() && lIAV != aA && lIAV != aB)
	{
		Link(lIAV, lOB);
	}
	SetBisectorSlope(aA, aB);
	if (lOAV->has_infinite_time())
	{
		EraseNode(lOAV);
	}
	if (lOBV->has_infinite_time())
	{
		EraseNode(lOBV);
	}
}
void SSkeletonBuilder::CollectNewEvents(Vertex* aNode, Triedge const& aPrevEventTriedge)
{
	Vertex* lPrev = GetPrevInLAV(aNode);
	Vertex* lNext = GetNextInLAV(aNode);
	if (IsReflex(aNode))
		CollectSplitEvents(aNode, aPrevEventTriedge);
	Event* lLEdgeEvent = FindEdgeEvent(lPrev, aNode, aPrevEventTriedge);
	Event* lREdgeEvent = FindEdgeEvent(aNode, lNext, aPrevEventTriedge);
	bool lAcceptL = !!lLEdgeEvent;
	bool lAcceptR = !!lREdgeEvent;
	if (lAcceptL)
		InsertEventInPQ(lLEdgeEvent);
	if (lAcceptR)
		InsertEventInPQ(lREdgeEvent);
}
void SSkeletonBuilder::InsertEventInPQ(Event* aEvent)
{
	mPQ.push(aEvent);
}
void SSkeletonBuilder::CollectSplitEvent(Vertex* aNode, Triedge const& aTriedge)
{
	if (IsOppositeEdgeFacingTheSplitSeed(aNode, aTriedge.e2()))
	{
		Trisegment* lTrisegment = CreateTrisegment(aTriedge, aNode);
		if (lTrisegment->collinearity() != TRISEGMENT_COLLINEARITY_02 && ExistEvent(lTrisegment))
		{
			if (CompareEvents(lTrisegment, aNode) != SMALLER)
			{
				Event* lEvent = GCNEW(SplitEvent(aTriedge, lTrisegment, aNode));
				AddSplitEvent(aNode, lEvent);
			}
		}
	}
}
void SSkeletonBuilder::CollectSplitEvents(Vertex* aNode, Triedge const& aPrevEventTriedge)
{
	// lLBorder and lRBorder are the consecutive contour edges forming the reflex wavefront.
	Triedge const& lTriedge = GetVertexTriedge(aNode);
	HalfEdge* lLBorder = lTriedge.e0();
	HalfEdge* lRBorder = lTriedge.e1();
	for (HalfEdgeIterator i = mContourHalfedges.begin(); i != mContourHalfedges.end(); ++i)
	{
		HalfEdge* lOpposite = *i;
		if (lOpposite != lLBorder && lOpposite != lRBorder)
		{
			Triedge lEventTriedge(lLBorder, lRBorder, lOpposite);
			if (lEventTriedge != aPrevEventTriedge)
			{
				CollectSplitEvent(aNode, lEventTriedge);
			}
		}
	}
}
Event* SSkeletonBuilder::FindEdgeEvent(Vertex* aLNode, Vertex* aRNode, Triedge const& aPrevEventTriedge)
{
	Event* rResult = NULL;
	Triedge lTriedge = GetVertexTriedge(aLNode) & GetVertexTriedge(aRNode);
	if (lTriedge.is_valid() && lTriedge != aPrevEventTriedge)
	{
		Trisegment* lTrisegment = CreateTrisegment(lTriedge, aLNode, aRNode);
		if (ExistEvent(lTrisegment))
		{
			Sign lLNodeD = CompareEvents(lTrisegment, aLNode);
			Sign lRNodeD = CompareEvents(lTrisegment, aRNode);
			if (lLNodeD != SMALLER && lRNodeD != SMALLER)
			{
				rResult = GCNEW(EdgeEvent(lTriedge, lTrisegment, aLNode, aRNode));
			}
			else
			{
			}
		}
	}
	return rResult;
}
void SSkeletonBuilder::Propagate()
{
	for (;;)
	{
		InsertNextSplitEventsInPQ();
		if (!mPQ.empty())
		{
			Event* lEvent = PopEventFromPQ();
			if (lEvent->type() != Event::cEdgeEvent)
				AllowNextSplitEvent(lEvent->seed0());
			if (!IsProcessed(lEvent))
			{
				SetEventTimeAndPoint(*lEvent);
				switch (lEvent->type())
				{
				case Event::cEdgeEvent: HandleEdgeEvent(lEvent); break;
				case Event::cSplitEvent: HandleSplitOrPseudoSplitEvent(lEvent); break;
				case Event::cPseudoSplitEvent: HandlePseudoSplitEvent(lEvent); break;
				}
				++mStepID;
			}
		}
		else
			break;
	}
}
void SSkeletonBuilder::InsertNextSplitEventsInPQ()
{
	for (VertexIterator v = mReflexVertices.begin(), ev = mReflexVertices.end(); v != ev; ++v)
	{
		if (!IsProcessed(*v))
			InsertNextSplitEventInPQ(*v);
	}
}
void SSkeletonBuilder::InsertNextSplitEventInPQ(Vertex* v)
{
	Event* lSplitEvent = PopNextSplitEvent(v);
	if (!!lSplitEvent) InsertEventInPQ(lSplitEvent);
}
void SSkeletonBuilder::EraseNode(Vertex* aNode)
{
	aNode->reset_id__internal__(-aNode->id());
	mSSkel->vertices_erase(aNode);
}
Event* SSkeletonBuilder::PopEventFromPQ()
{
	Event* rR = mPQ.top(); mPQ.pop();
	return rR;
}
VertexPair SSkeletonBuilder::LookupOnSLAV(HalfEdge* aBorder, Event* const& aEvent, Site& rSite)
{
	VertexPair rResult;
//	Vertex_handle lSeed = aEvent->seed0();
	for (std::list<Vertex*>::const_iterator vi = mGLAV.begin(); vi != mGLAV.end(); ++vi)
	{
		Vertex* v = *vi;
		Triedge const& lTriedge = GetVertexTriedge(v);
		Vertex* lPrevN = GetPrevInLAV(v);
		Vertex* lNextN = GetNextInLAV(v);
		if (lTriedge.e0() == aBorder)
		{
			HalfEdge* lPrevBorder = GetEdgeEndingAt(lPrevN);
			HalfEdge* lNextBorder = GetEdgeEndingAt(lNextN);
			Sign lLSide = EventPointOrientedSide(*aEvent, lPrevBorder, aBorder, lPrevN, false);
			Sign lRSide = EventPointOrientedSide(*aEvent, aBorder, lNextBorder, v, true);
			if (lLSide != ON_POSITIVE_SIDE && lRSide != ON_NEGATIVE_SIDE)
			{
				if (lLSide != ON_ORIENTED_BOUNDARY || lRSide != ON_ORIENTED_BOUNDARY)
				{
					rSite = (lLSide == ON_ORIENTED_BOUNDARY ? AT_SOURCE : (lRSide == ON_ORIENTED_BOUNDARY ? AT_TARGET : INSIDE));
					rResult = std::make_pair(lPrevN, v);
					break;
				}
				else
				{
				}
			}
		}
	}
	return rResult;
}
void SSkeletonBuilder::HandleEdgeEvent(Event* aEvent)
{
	EdgeEvent& lEvent = static_cast<EdgeEvent&>(*aEvent);
	if (IsValidEdgeEvent(lEvent))
	{
		Vertex* lLSeed = lEvent.seed0();
		Vertex* lRSeed = lEvent.seed1();
		Vertex* lNewNode = ConstructEdgeEventNode(lEvent);
		HalfEdge* lLOBisector = lLSeed->primary_bisector();
		HalfEdge* lROBisector = lRSeed->primary_bisector();
		HalfEdge* lLIBisector = lLOBisector->opposite();
		HalfEdge* lRIBisector = lROBisector->opposite();
		Vertex* lRIFicNode = lROBisector->vertex();
		Vertex* lLOFicNode = lLOBisector->vertex();
		CrossLink(lLOBisector, lNewNode);
		Link(lROBisector, lNewNode);
		CrossLinkFwd(lROBisector, lLIBisector);
		HalfEdge* lDefiningBorderA = lNewNode->halfedge()->defining_contour_edge();
		HalfEdge* lDefiningBorderB = lNewNode->halfedge()->opposite()->prev()->opposite()->defining_contour_edge();
		HalfEdge* lDefiningBorderC = lNewNode->halfedge()->opposite()->prev()->defining_contour_edge();
		lNewNode->set_event_triedge(lEvent.triedge());
		Triedge lTri(lDefiningBorderA, lDefiningBorderB, lDefiningBorderC);
		SetVertexTriedge(lNewNode, lTri);
		SetBisectorSlope(lLSeed, lNewNode);
		SetBisectorSlope(lRSeed, lNewNode);
		if (lLOFicNode->has_infinite_time())
		{
			HalfEdge* lNOBisector = mSSkel->edges_push_back(GCNEW(HalfEdge(mEdgeID)), GCNEW(HalfEdge(mEdgeID + 1)));
			HalfEdge* lNIBisector = lNOBisector->opposite();
			mEdgeID += 2;
			CrossLinkFwd(lNOBisector, lLOBisector->next());
			CrossLinkFwd(lRIBisector->prev(), lNIBisector);
			CrossLink(lNOBisector, lLOFicNode);
			SetBisectorSlope(lNOBisector, POSITIVE);
			SetBisectorSlope(lNIBisector, NEGATIVE);
			CrossLinkFwd(lNIBisector, lRIBisector);
			CrossLinkFwd(lLOBisector, lNOBisector);
			Link(lNOBisector, lLOBisector->face());
			Link(lNIBisector, lRIBisector->face());
			Link(lNIBisector, lNewNode);
			EraseNode(lRIFicNode);
			SetupNewNode(lNewNode);
			UpdatePQ(lNewNode, lEvent.triedge());
		}
		else
		{
		}
	}
}
void SSkeletonBuilder::SetupNewNode(Vertex* aNode)
{
	// In an edge-edge anihiliation the current polygon becomes a two-node degenerate chain collapsed into a single point
	if (GetPrevInLAV(aNode) != GetNextInLAV(aNode))
	{
		HalfEdge* lLE = GetEdgeEndingAt(aNode);
		HalfEdge* lRE = GetEdgeStartingAt(aNode);
		Point2D lLV = CreateVector(lLE);
		Point2D lRV = CreateVector(lRE);
		Sign lOrientation = orientation(lLV, lRV);
		if (lOrientation == COLLINEAR)
		{
			SetIsDegenerate(aNode);
		}
		else if (lOrientation == RIGHT_TURN)
		{
			mReflexVertices.push_back(aNode);
			SetIsReflex(aNode);
		}
	}
}
Vertex* SSkeletonBuilder::ConstructEdgeEventNode(EdgeEvent& aEvent)
{
	Vertex* lLSeed = aEvent.seed0();
	Vertex* lRSeed = aEvent.seed1();
	Vertex* lNewNode = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++, aEvent.point(), aEvent.time(), false, false)));
	InitVertexData(lNewNode);
	mGLAV.push_back(lNewNode);
	SetTrisegment(lNewNode, aEvent.trisegment());
	SetIsProcessed(lLSeed);
	SetIsProcessed(lRSeed);
	mGLAV.remove(lLSeed);
	mGLAV.remove(lRSeed);
	Vertex* lLPrev = GetPrevInLAV(lLSeed);
	Vertex* lRNext = GetNextInLAV(lRSeed);
	SetPrevInLAV(lNewNode, lLPrev);
	SetNextInLAV(lLPrev, lNewNode);
	SetNextInLAV(lNewNode, lRNext);
	SetPrevInLAV(lRNext, lNewNode);
	return lNewNode;
}
VertexPair SSkeletonBuilder::ConstructSplitEventNodes(SplitEvent& aEvent, Vertex* aOppR)
{
	VertexPair rResult;
	Vertex* lOppL = GetPrevInLAV(aOppR);
	Vertex* lNewNodeA = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++, aEvent.point(), aEvent.time(), true, false)));
	Vertex* lNewNodeB = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++, aEvent.point(), aEvent.time(), true, false)));
	InitVertexData(lNewNodeA);
	InitVertexData(lNewNodeB);
	SetTrisegment(lNewNodeA, aEvent.trisegment());
	SetTrisegment(lNewNodeB, aEvent.trisegment());
	mGLAV.push_back(lNewNodeA);
	mGLAV.push_back(lNewNodeB);
	Vertex* lSeed = aEvent.seed0();
	SetIsProcessed(lSeed);
	mGLAV.remove(lSeed);
	Vertex* lPrev = GetPrevInLAV(lSeed);
	Vertex* lNext = GetNextInLAV(lSeed);
	SetNextInLAV(lPrev, lNewNodeA);
	SetPrevInLAV(lNewNodeA, lPrev);
	SetNextInLAV(lNewNodeA, aOppR);
	SetPrevInLAV(aOppR, lNewNodeA);
	SetNextInLAV(lOppL, lNewNodeB);
	SetPrevInLAV(lNewNodeB, lOppL);
	SetNextInLAV(lNewNodeB, lNext);
	SetPrevInLAV(lNext, lNewNodeB);
	rResult = std::make_pair(lNewNodeA, lNewNodeB);
	mSplitNodes.push_back(rResult);
	return rResult;
}
bool SSkeletonBuilder::IsValidEdgeEvent(EdgeEvent const& aEvent)
{
	bool rResult = false;
	Vertex* lLSeed = aEvent.seed0();
	Vertex* lRSeed = aEvent.seed1();
	Vertex* lPrevLSeed = GetPrevInLAV(lLSeed);
	Vertex* lNextRSeed = GetNextInLAV(lRSeed);
	if (lPrevLSeed != lNextRSeed)
	{
		HalfEdge* lPrevE0 = GetEdgeEndingAt(lPrevLSeed);
		HalfEdge* lE0 = aEvent.triedge().e0();
		HalfEdge* lE2 = aEvent.triedge().e2();
		HalfEdge* lNextE2 = GetEdgeStartingAt(lNextRSeed);
		Sign lLSide = EventPointOrientedSide(aEvent, lPrevE0, lE0, lPrevLSeed, false);
		Sign lRSide = EventPointOrientedSide(aEvent, lE2, lNextE2, lNextRSeed, true);
		bool lLSideOK = (lLSide != ON_POSITIVE_SIDE);
		bool lRSideOK = (lRSide != ON_NEGATIVE_SIDE);
		rResult = lLSideOK && lRSideOK;
	}
	else
	{
		// Triangle collapse. No need to test explicitely.
		rResult = true;
	}
	return rResult;
}
void SSkeletonBuilder::HandlePseudoSplitEvent(Event* aEvent)
{
	PseudoSplitEvent& lEvent = static_cast<PseudoSplitEvent&>(*aEvent);
	if (IsValidPseudoSplitEvent(lEvent))
	{
		Vertex* lLSeed = lEvent.seed0();
		Vertex* lRSeed = lEvent.seed1();
		VertexPair t = ConstructPseudoSplitEventNodes(lEvent);
		Vertex* lNewNode_L = t.first, *lNewNode_R = t.second;
		HalfEdge* lNBisector_LO = mSSkel->edges_push_back(GCNEW(HalfEdge(mEdgeID)), GCNEW(HalfEdge(mEdgeID + 1)));
		HalfEdge* lNBisector_RO = mSSkel->edges_push_back(GCNEW(HalfEdge(mEdgeID + 2)), GCNEW(HalfEdge(mEdgeID + 3)));
		HalfEdge* lNBisector_LI = lNBisector_LO->opposite();
		HalfEdge* lNBisector_RI = lNBisector_RO->opposite();
		mEdgeID += 4;
		HalfEdge* lSBisector_LO = lLSeed->primary_bisector();
		HalfEdge* lSBisector_LI = lSBisector_LO->opposite();
		HalfEdge* lSBisector_RO = lRSeed->primary_bisector();
		HalfEdge* lSBisector_RI = lSBisector_RO->opposite();
		HalfEdge* lSBisector_LO_Next = lSBisector_LO->next();
		HalfEdge* lSBisector_RO_Next = lSBisector_RO->next();
		HalfEdge* lSBisector_LI_Prev = lSBisector_LI->prev();
		HalfEdge* lSBisector_RI_Prev = lSBisector_RI->prev();
		Vertex* lFicNod_SLO = lSBisector_LO->vertex();
		Vertex* lFicNod_SRO = lSBisector_RO->vertex();
		Link(lNBisector_LO, lSBisector_LO->face());
		Link(lNBisector_LI, lSBisector_RI->face());
		Link(lNBisector_RO, lSBisector_RO->face());
		Link(lNBisector_RI, lSBisector_LI->face());
		CrossLink(lSBisector_LO, lNewNode_L);
		CrossLink(lSBisector_RO, lNewNode_R);
		CrossLink(lNBisector_LO, lFicNod_SLO);
		CrossLink(lNBisector_RO, lFicNod_SRO);
		SetBisectorSlope(lNBisector_LO, POSITIVE);
		SetBisectorSlope(lNBisector_LI, NEGATIVE);
		SetBisectorSlope(lNBisector_RO, POSITIVE);
		SetBisectorSlope(lNBisector_RI, NEGATIVE);
		Link(lNBisector_LI, lNewNode_L);
		Link(lNBisector_RI, lNewNode_R);
		Link(lNewNode_L, lSBisector_LO);
		Link(lNewNode_R, lSBisector_RO);
		CrossLinkFwd(lSBisector_LO, lNBisector_LO);
		CrossLinkFwd(lNBisector_LO, lSBisector_LO_Next);
		CrossLinkFwd(lSBisector_LI_Prev, lNBisector_RI);
		CrossLinkFwd(lNBisector_RI, lSBisector_LI);
		CrossLinkFwd(lSBisector_RI_Prev, lNBisector_LI);
		CrossLinkFwd(lNBisector_LI, lSBisector_RI);
		CrossLinkFwd(lSBisector_RO, lNBisector_RO);
		CrossLinkFwd(lNBisector_RO, lSBisector_RO_Next);
		SetBisectorSlope(lLSeed, lNewNode_L);
		SetBisectorSlope(lRSeed, lNewNode_R);
		HalfEdge* lNewNode_L_DefiningBorderA = lNewNode_L->halfedge()->defining_contour_edge();
		HalfEdge* lNewNode_L_DefiningBorderB = lNewNode_L->halfedge()->next()->opposite()->defining_contour_edge();
		HalfEdge* lNewNode_L_DefiningBorderC = lNewNode_L->halfedge()->opposite()->prev()->defining_contour_edge();
		HalfEdge* lNewNode_R_DefiningBorderA = lNewNode_R->halfedge()->defining_contour_edge();
		HalfEdge* lNewNode_R_DefiningBorderB = lNewNode_R->halfedge()->next()->opposite()->defining_contour_edge();
		HalfEdge* lNewNode_R_DefiningBorderC = lNewNode_R->halfedge()->opposite()->prev()->defining_contour_edge();
		lNewNode_L->set_event_triedge(lEvent.triedge());
		lNewNode_R->set_event_triedge(lEvent.triedge());
		Triedge lTriL(lNewNode_L_DefiningBorderA, lNewNode_L_DefiningBorderB, lNewNode_L_DefiningBorderC);
		Triedge lTriR(lNewNode_R_DefiningBorderA, lNewNode_R_DefiningBorderB, lNewNode_R_DefiningBorderC);
		SetVertexTriedge(lNewNode_L, lTriL);
		SetVertexTriedge(lNewNode_R, lTriR);
		SetupNewNode(lNewNode_L);
		SetupNewNode(lNewNode_R);
		UpdatePQ(lNewNode_L, lEvent.triedge());
		UpdatePQ(lNewNode_R, lEvent.triedge());
	}
}
VertexPair SSkeletonBuilder::ConstructPseudoSplitEventNodes(PseudoSplitEvent& aEvent)
{
	VertexPair rResult;
	Vertex* lLSeed = aEvent.seed0();
	Vertex* lRSeed = aEvent.seed1();
	Vertex* lNewNodeA = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++, aEvent.point(), aEvent.time(), true, false)));
	Vertex* lNewNodeB = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++, aEvent.point(), aEvent.time(), true, false)));
	mGLAV.push_back(lNewNodeA);
	mGLAV.push_back(lNewNodeB);
	InitVertexData(lNewNodeA);
	InitVertexData(lNewNodeB);
	SetTrisegment(lNewNodeA, aEvent.trisegment());
	SetTrisegment(lNewNodeB, aEvent.trisegment());
	SetIsProcessed(lLSeed);
	SetIsProcessed(lRSeed);
	mGLAV.remove(lLSeed);
	mGLAV.remove(lRSeed);
	Vertex* lLPrev = GetPrevInLAV(lLSeed);
	Vertex* lLNext = GetNextInLAV(lLSeed);
	Vertex* lRPrev = GetPrevInLAV(lRSeed);
	Vertex* lRNext = GetNextInLAV(lRSeed);
	SetPrevInLAV(lNewNodeA, lLPrev);
	SetNextInLAV(lLPrev, lNewNodeA);
	SetNextInLAV(lNewNodeA, lRNext);
	SetPrevInLAV(lRNext, lNewNodeA);
	SetPrevInLAV(lNewNodeB, lRPrev);
	SetNextInLAV(lRPrev, lNewNodeB);
	SetNextInLAV(lNewNodeB, lLNext);
	SetPrevInLAV(lLNext, lNewNodeB);
	rResult = std::make_pair(lNewNodeA, lNewNodeB);
	mSplitNodes.push_back(rResult);
	return rResult;
}
bool counterclockwise_in_between(const Point2D& p, const Point2D& q, const Point2D& r)
{
	if (q < p) return (p < r) || (r <= q);
	else return (p < r) && (r <= q);
}
bool counterclockwise_at_or_in_between_2(Point2D const& p, Point2D const& q, Point2D const& r)
{
//	typedef typename Kernel_traits<Direction>::Kernel K;
	return p == q || p == r || counterclockwise_in_between(p, q, r);
}
bool SSkeletonBuilder::IsValidPseudoSplitEvent(PseudoSplitEvent const& aEvent)
{
	Vertex* lSeed0 = aEvent.seed0();
	Vertex* lSeed1 = aEvent.seed1();
	HalfEdge* lEL0 = GetEdgeEndingAt(lSeed0);
	HalfEdge* lER0 = GetEdgeStartingAt(lSeed0);
	HalfEdge* lEL1 = GetEdgeEndingAt(lSeed1);
	HalfEdge* lER1 = GetEdgeStartingAt(lSeed1);
	Point2D lDL0 = -CreateDirection(lEL0);
	Point2D lDL1 = -CreateDirection(lEL1);
	Point2D lDR0 = CreateDirection(lER0);
	Point2D lDR1 = CreateDirection(lER1);
	bool lV01Degenerate = (lDL0 == lDR1);
	bool lV10Degenerate = (lDL1 == lDR0);
	bool lTangled;
	if (!lV01Degenerate)
	{
		bool lEL1V_Tangled = counterclockwise_at_or_in_between_2(lDL1, lDR1, lDL0);
		bool lER0V_Tangled = counterclockwise_at_or_in_between_2(lDR0, lDR1, lDL0);
		lTangled = lEL1V_Tangled || lER0V_Tangled;
	}
	else if (!lV10Degenerate)
	{
		bool lEL0V_Tangled = counterclockwise_at_or_in_between_2(lDL0, lDR0, lDL1);
		bool lER1V_Tangled = counterclockwise_at_or_in_between_2(lDR1, lDR0, lDL1);
		lTangled = lEL0V_Tangled || lER1V_Tangled;
	}
	else
	{
		lTangled = (lDL0 == lDL1);
	}
	return !lTangled;
}
void SSkeletonBuilder::HandleSplitOrPseudoSplitEvent(Event* aEvent)
{
	HalfEdge* lOppEdge = aEvent->triedge().e2();
	Site lSite;
	VertexPair lOpp = LookupOnSLAV(lOppEdge, aEvent, lSite);
	if (lOpp.first)
	{
		Event* lPseudoSplitEvent = IsPseudoSplitEvent(aEvent, lOpp, lSite);
		if (lPseudoSplitEvent)
			HandlePseudoSplitEvent(lPseudoSplitEvent);
		else
			HandleSplitEvent(aEvent, lOpp);
	}
}
void SSkeletonBuilder::HandleSplitEvent(Event* aEvent, VertexPair aOpp)
{
	SplitEvent& lEvent = static_cast<SplitEvent&>(*aEvent);
	if (IsValidSplitEvent(lEvent))
	{
		Vertex* lSeed = lEvent.seed0();
		Vertex* lOppL = aOpp.first;
		Vertex* lOppR = aOpp.second;
		HalfEdge* lOppIBisector_L = lOppL->primary_bisector()->opposite();
		HalfEdge* lOppOBisector_R = lOppR->primary_bisector();
		Vertex* lOppFicNode = lOppOBisector_R->vertex();
		(void)lOppFicNode; // variable may be unused
		HalfEdge* lOppBorder = lEvent.triedge().e2();
		VertexPair t = ConstructSplitEventNodes(lEvent, lOppR);
		Vertex* lNewNode_L = t.first, *lNewNode_R = t.second;
	// Triedge lTriedge = aEvent->triedge();
	// Halfedge_handle lReflexLBorder = lTriedge.e0();
	// Halfedge_handle lReflexRBorder = lTriedge.e1();
		HalfEdge* lNOBisector_L = mSSkel->edges_push_back(GCNEW(HalfEdge(mEdgeID)), GCNEW(HalfEdge(mEdgeID + 1)));
		HalfEdge* lNOBisector_R = mSSkel->edges_push_back(GCNEW(HalfEdge(mEdgeID + 2)), GCNEW(HalfEdge(mEdgeID + 3)));
		HalfEdge* lNIBisector_L = lNOBisector_L->opposite();
		HalfEdge* lNIBisector_R = lNOBisector_R->opposite();
		mEdgeID += 4;
		HalfEdge* lXOBisector = lSeed->primary_bisector();
		HalfEdge* lXIBisector = lXOBisector->opposite();
		HalfEdge* lXONextBisector = lXOBisector->next();
		HalfEdge* lXIPrevBisector = lXIBisector->prev();
		Vertex* lXOFicNode = lXOBisector->vertex();
		Link(lNewNode_L, lXOBisector);
		Link(lNewNode_R, lNIBisector_L);
		Link(lXOBisector, lNewNode_L);
		Link(lNOBisector_L, lXOBisector->face());
		Link(lNIBisector_L, lOppBorder->face());
		Link(lNOBisector_R, lOppBorder->face());
		Link(lNIBisector_R, lXIBisector->face());
		Link(lNIBisector_L, lNewNode_R);
		Link(lNIBisector_R, lNewNode_R);
		Link(lNOBisector_L, lXOFicNode);
		CrossLinkFwd(lXOBisector, lNOBisector_L);
		CrossLinkFwd(lNOBisector_L, lXONextBisector);
		CrossLinkFwd(lXIPrevBisector, lNIBisector_R);
		CrossLinkFwd(lNIBisector_R, lXIBisector);
		CrossLinkFwd(lOppOBisector_R, lNIBisector_L);
		CrossLinkFwd(lNIBisector_L, lNOBisector_R);
		CrossLinkFwd(lNOBisector_R, lOppIBisector_L);
		SetBisectorSlope(lSeed, lNewNode_L);
		Vertex* lNewFicNode = mSSkel->vertices_push_back(GCNEW(Vertex(mVertexID++)));
		InitVertexData(lNewFicNode);
		CrossLink(lNOBisector_R, lNewFicNode);
		SetBisectorSlope(lNOBisector_L, POSITIVE);
		SetBisectorSlope(lNIBisector_L, NEGATIVE);
		SetBisectorSlope(lNOBisector_R, POSITIVE);
		SetBisectorSlope(lNIBisector_R, NEGATIVE);
		HalfEdge* lNewNode_L_DefiningBorderA = lNewNode_L->halfedge()->defining_contour_edge();
		HalfEdge* lNewNode_L_DefiningBorderB = lNewNode_L->halfedge()->opposite()->prev()->opposite()->defining_contour_edge();
		HalfEdge* lNewNode_L_DefiningBorderC = lNewNode_L->halfedge()->opposite()->prev()->defining_contour_edge();
		HalfEdge* lNewNode_R_DefiningBorderA = lNewNode_R->halfedge()->defining_contour_edge();
		HalfEdge* lNewNode_R_DefiningBorderB = lNewNode_R->halfedge()->opposite()->prev()->opposite()->defining_contour_edge();
		HalfEdge* lNewNode_R_DefiningBorderC = lNewNode_R->halfedge()->opposite()->prev()->defining_contour_edge();
		lNewNode_L->set_event_triedge(lEvent.triedge());
		lNewNode_R->set_event_triedge(lEvent.triedge());
		Triedge lTriL(lNewNode_L_DefiningBorderA, lNewNode_L_DefiningBorderB, lNewNode_L_DefiningBorderC);
		Triedge lTriR(lNewNode_R_DefiningBorderA, lNewNode_R_DefiningBorderB, lNewNode_R_DefiningBorderC);
		SetVertexTriedge(lNewNode_L, lTriL);
		SetVertexTriedge(lNewNode_R, lTriR);
		SetupNewNode(lNewNode_L);
		SetupNewNode(lNewNode_R);
		UpdatePQ(lNewNode_L, lEvent.triedge());
		UpdatePQ(lNewNode_R, lEvent.triedge());
	}
}
Event* SSkeletonBuilder::IsPseudoSplitEvent(Event* const& aEvent, VertexPair aOpp, Site const& aSite)
{
	Event* rPseudoSplitEvent = NULL;
	if (aSite != INSIDE)
	{
		SplitEvent& lEvent = static_cast<SplitEvent&>(*aEvent);
		Triedge const& lEventTriedge = lEvent.triedge();
		Trisegment* const& lEventTrisegment = lEvent.trisegment();
		Vertex* lSeedN = lEvent.seed0();
		Vertex* lOppL = aOpp.first;
		Vertex* lOppR = aOpp.second;
		if (aSite == AT_SOURCE)
		{
			HalfEdge* lOppPrevBorder = GetVertexTriedge(lOppL).e0();
			if (lEventTriedge.e0() != lOppPrevBorder && lEventTriedge.e1() != lOppPrevBorder)
			{
				rPseudoSplitEvent = GCNEW(PseudoSplitEvent(lEventTriedge, lEventTrisegment, lOppL, lSeedN, true));
			}
		}
		else // aSite == AT_TARGET 
		{
			Vertex* lOppNextN = GetNextInLAV(lOppR);
			HalfEdge* lOppNextBorder = GetVertexTriedge(lOppNextN).e0();
			if (lEventTriedge.e0() != lOppNextBorder && lEventTriedge.e1() != lOppNextBorder)
			{
				rPseudoSplitEvent = GCNEW(PseudoSplitEvent(lEventTriedge, lEventTrisegment, lSeedN, lOppR, false));
			}
		}
	}
	if (rPseudoSplitEvent)
		rPseudoSplitEvent->SetTimeAndPoint(aEvent->time(), aEvent->point());
	return rPseudoSplitEvent;
}
bool SSkeletonBuilder::IsProcessed(Event* aEvent)
{
	return IsProcessed(aEvent->seed0()) || IsProcessed(aEvent->seed1());
}
bool SSkeletonBuilder::IsValidSplitEvent(SplitEvent const& /*aEvent*/)
{
	return true;
}
SSkeleton* SSkeletonBuilder::construct_skeleton(bool aNull_if_failed)
{
	bool ok = false;
	try
	{
		ok = Run();
	}
	catch (...)
	{
	}
	if (!ok)
	{
		if (aNull_if_failed)
			mSSkel = NULL;
	}
	return mSSkel;
}