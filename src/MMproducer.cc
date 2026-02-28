#include "MMproducer.h"

// Headers for services and tools
// #include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
// #include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "RecoVertex/KalmanVertexFit/interface/KalmanVertexFitter.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
#include "TMath.h"
#include "Math/VectorUtil.h"
#include "TVector3.h"

// #include "MagneticField/Engine/interface/MagneticField.h"
// #include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "TrackingTools/PatternTools/interface/TwoTrackMinimumDistance.h"
#include "TrackingTools/PatternTools/interface/ClosestApproachInRPhi.h"

MMproducer::MMproducer(const edm::ParameterSet &iConfig) : muons_(consumes<pat::MuonCollection>(iConfig.getParameter<edm::InputTag>("muons"))),
														   thebeamspot_(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot"))),
														   thePVs_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("primaryvertices"))),
														   magneticField_(esConsumes<MagneticField, IdealMagneticFieldRecord>()),
														   theTTBuilder_(esConsumes<TransientTrackBuilder, TransientTrackRecord>(edm::ESInputTag("", "TransientTrackBuilder"))),
														   dimuonSelection_(iConfig.existsAs<std::string>("dimuonselection") ? iConfig.getParameter<std::string>("dimuonselection") : ""),
														   resolveAmbiguity_(iConfig.getParameter<bool>("resolvePUambiguity"))
{
	produces<pat::CompositeCandidateCollection>();
}

void MMproducer::produce(edm::Event &iEvent, const edm::EventSetup &iSetup)
{

	std::vector<double> muMasses;
	muMasses.push_back(0.1056583715);
	muMasses.push_back(0.1056583715);

	std::unique_ptr<pat::CompositeCandidateCollection> oniaOutput(new pat::CompositeCandidateCollection);

	reco::Vertex thePrimaryV;
	reco::Vertex theBeamSpotV;

	const MagneticField &magneticField = iSetup.getData(magneticField_);

	// edm::ESHandle<MagneticField> magneticField;
	// iSetup.get<IdealMagneticFieldRecord>().get(magneticField);

	edm::Handle<reco::BeamSpot> theBeamSpot;
	iEvent.getByToken(thebeamspot_, theBeamSpot);
	reco::BeamSpot bs = *theBeamSpot;
	theBeamSpotV = reco::Vertex(bs.position(), bs.covariance3D());

	edm::Handle<reco::VertexCollection> priVtxs;
	iEvent.getByToken(thePVs_, priVtxs);
	if (priVtxs->begin() != priVtxs->end())
		thePrimaryV = reco::Vertex(*(priVtxs->begin()));
	else
		thePrimaryV = reco::Vertex(bs.position(), bs.covariance3D());

	edm::Handle<pat::MuonCollection> muons;
	iEvent.getByToken(muons_, muons);

	edm::ESHandle<TransientTrackBuilder> theTTBuilder = iSetup.getHandle(theTTBuilder_);

	// edm::ESHandle<TransientTrackBuilder> theTTBuilder;
	// iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder", theTTBuilder);
	//
	KalmanVertexFitter vtxFitter(true);

	// Dimuon candidates only from muons
	int i_mm = 0;
	for (pat::MuonCollection::const_iterator it = muons->begin(), itend = muons->end(); it != itend; ++it)
	{
		for (pat::MuonCollection::const_iterator it2 = muons->begin(); it2 != itend; ++it2)
		{
			if (it == it2)
				continue;
			if (it2->pt() < it->pt())
				continue;

			pat::CompositeCandidate mmCandidate; // mumu事例

			mmCandidate.addDaughter(*it, "muon1"); // unsorted
			mmCandidate.addDaughter(*it2, "muon2");

			mmCandidate.setP4(it->p4() + it2->p4());
			mmCandidate.setCharge(it->charge() + it2->charge());

			if (!dimuonSelection_(mmCandidate))
				continue; // ---- apply the dimuon cut ----

			// ---- fit vertex using Tracker tracks (if they have tracks) ----
			float vProb = -1;
			if (it->track().isNonnull() && it2->track().isNonnull())
			{ // build the dimuon secondary vertex
				std::vector<reco::TransientTrack> t_tks;
				t_tks.push_back(theTTBuilder->build(*it->track()));	 // pass the reco::Track, not  the reco::TrackRef (which can be transient)
				t_tks.push_back(theTTBuilder->build(*it2->track())); // otherwise the vertex will have transient refs inside.
				TransientVertex myVertex = vtxFitter.vertex(t_tks);	 // mumu轨迹顶点拟合
				if (myVertex.isValid())
				{
					float vChi2 = myVertex.totalChiSquared(); // 轨迹距离	顶点好坏
					float vNDF = myVertex.degreesOfFreedom();
					vProb = TMath::Prob(vChi2, (int)vNDF);
					mmCandidate.addUserFloat("vNChi2", vChi2 / vNDF);

					if (resolveAmbiguity_)
					{ // find the closest PV in z, to the extrapolate MM to beam axis
						float minDz = 999999.;
						TwoTrackMinimumDistance ttmd;
						bool status = ttmd.calculate(
							GlobalTrajectoryParameters(GlobalPoint(myVertex.position().x(), myVertex.position().y(), myVertex.position().z()),
													   GlobalVector(mmCandidate.px(), mmCandidate.py(), mmCandidate.pz()),
													   TrackCharge(0), &(magneticField)),
							GlobalTrajectoryParameters(GlobalPoint(bs.position().x(), bs.position().y(), bs.position().z()),
													   GlobalVector(bs.dxdz(), bs.dydz(), 1.),
													   TrackCharge(0), &(magneticField)));
						float extrapZ = -9999999.;
						if (status)
							extrapZ = ttmd.points().first.z();

						for (reco::VertexCollection::const_iterator itv = priVtxs->begin(), itvend = priVtxs->end(); itv != itvend; ++itv)
						{ // 找到最近的z方向轨迹 匹配为mu子对
							float deltaZ = fabs(extrapZ - itv->position().z());
							if (deltaZ < minDz)
							{
								minDz = deltaZ;
								thePrimaryV = reco::Vertex(*itv);
							}
						}
					}

					mmCandidate.addUserData("PVwithmuons", thePrimaryV);

					// DCA
					TrajectoryStateClosestToPoint mu1TS = t_tks[0].impactPointTSCP();
					TrajectoryStateClosestToPoint mu2TS = t_tks[1].impactPointTSCP();
					float dca = -1.;
					if (mu1TS.isValid() && mu2TS.isValid())
					{
						ClosestApproachInRPhi cApp;
						cApp.calculate(mu1TS.theState(), mu2TS.theState());
						if (cApp.status())
							dca = cApp.distance();
					}
					mmCandidate.addUserFloat("DCA", dca);

					// lifetime using PV
					TVector3 vtx;
					TVector3 pvtx;
					VertexDistanceXY vdistXY;

					vtx.SetXYZ(myVertex.position().x(), myVertex.position().y(), 0);
					TVector3 pperp(mmCandidate.px(), mmCandidate.py(), 0);
					AlgebraicVector3 vpperp(pperp.x(), pperp.y(), 0);

					pvtx.SetXYZ(thePrimaryV.position().x(), thePrimaryV.position().y(), 0);
					TVector3 vdiff = vtx - pvtx;
					double cosAlpha = vdiff.Dot(pperp) / (vdiff.Perp() * pperp.Perp());
					Measurement1D distXY = vdistXY.distance(reco::Vertex(myVertex), thePrimaryV);
					double ctauPV = distXY.value() * cosAlpha * mmCandidate.mass() / pperp.Perp();
					GlobalError v1e = (reco::Vertex(myVertex)).error();
					GlobalError v2e = thePrimaryV.error();
					AlgebraicSymMatrix33 vXYe = v1e.matrix() + v2e.matrix();
					double ctauErrPV = sqrt(ROOT::Math::Similarity(vpperp, vXYe)) * mmCandidate.mass() / (pperp.Perp2());

					mmCandidate.addUserFloat("ppdlPV", ctauPV);
					mmCandidate.addUserFloat("ppdlErrPV", ctauErrPV);
					mmCandidate.addUserFloat("cosAlpha", cosAlpha);

					// lifetime using BS
					pvtx.SetXYZ(theBeamSpotV.position().x(), theBeamSpotV.position().y(), 0);
					vdiff = vtx - pvtx;
					double cosAlphaBS = vdiff.Dot(pperp) / (vdiff.Perp() * pperp.Perp());
					distXY = vdistXY.distance(reco::Vertex(myVertex), theBeamSpotV);
					double ctauBS = distXY.value() * cosAlphaBS * mmCandidate.mass() / pperp.Perp();
					GlobalError v1eB = (reco::Vertex(myVertex)).error();
					GlobalError v2eB = theBeamSpotV.error();
					AlgebraicSymMatrix33 vXYeB = v1eB.matrix() + v2eB.matrix();
					double ctauErrBS = sqrt(ROOT::Math::Similarity(vpperp, vXYeB)) * mmCandidate.mass() / (pperp.Perp2());

					mmCandidate.addUserFloat("ppdlBS", ctauBS);
					mmCandidate.addUserFloat("ppdlErrBS", ctauErrBS);
					mmCandidate.addUserFloat("cosAlphaBS", cosAlphaBS);

					mmCandidate.setVertex(reco::Vertex(myVertex).position());
					mmCandidate.addUserData("commonVertex", reco::Vertex(myVertex));

					mmCandidate.addUserFloat("vProb", vProb);
					oniaOutput->push_back(mmCandidate); // store candiate, even when no vertex

					i_mm++; // count number of good mm vertexes

				} // a valid mmVertex
			} //  both muon tracks not null
		} // muon2
	} // muon1

	if (i_mm != (int)oniaOutput->size())
		std::cout << "MMproducer::produce *** size and counter not match *** " << iEvent.id().run() << "," << iEvent.id().event() << std::endl;
	if (i_mm > 1)
		std::sort(oniaOutput->begin(), oniaOutput->end(), vPComparator_); // sort by vProb if any
	iEvent.put(std::move(oniaOutput));									  // store in the event
}

void MMproducer::fillDescriptions(edm::ConfigurationDescriptions &iDescriptions)
{
	edm::ParameterSetDescription desc;

	desc.add<edm::InputTag>("muons");
	desc.add<edm::InputTag>("beamspot");
	desc.add<edm::InputTag>("primaryvertices");
	desc.add<std::string>("dimuonselection");
	desc.add<bool>("resolvePUambiguity");
	// desc.add<uint32_t>("pdgid");

	iDescriptions.addDefault(desc);
}

DEFINE_FWK_MODULE(MMproducer);
