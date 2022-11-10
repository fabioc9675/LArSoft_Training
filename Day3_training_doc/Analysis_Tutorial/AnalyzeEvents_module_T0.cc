////////////////////////////////////////////////////////////////////////
// Class:       AnalyzeEvents
// Plugin Type: analyzer (Unknown Unknown)
// File:        AnalyzeEvents_module.cc
//
// Generated at Thu Nov 10 09:36:02 2022 by dune13 using cetskelgen
// from  version .
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Additional framework includes
#include "art_root_io/TFileService.h"
#include "canvas/Persistency/Common/FindManyP.h"

// Additonal LArSoft includes
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Slice.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "lardataobj/AnalysisBase/T0.h"

// ROOT includes
#include <TTree.h>
#include <TH1F.h>

// STL includes
#include <string>
#include <vector>

namespace test {
  class AnalyzeEvents;
}


class test::AnalyzeEvents : public art::EDAnalyzer {
public:
  explicit AnalyzeEvents(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  AnalyzeEvents(AnalyzeEvents const&) = delete;
  AnalyzeEvents(AnalyzeEvents&&) = delete;
  AnalyzeEvents& operator=(AnalyzeEvents const&) = delete;
  AnalyzeEvents& operator=(AnalyzeEvents&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

  // Selected optional functions.
  void beginJob() override;
  void endJob() override;

private:
  // Create output TTree
  TTree *fTree;
  // Create output histogram
  TH1F *fTrackLengthHist;  

  // Tree variables
  unsigned int fEventID;
  unsigned int fNPFParticles;
  unsigned int fNPrimaries;
  int fNPrimaryDaughters;

  std::vector<float> fDaughterTrackLengths;
  std::vector<std::vector<float>> fDaughterTrackdEdx;
  std::vector<std::vector<float>> fDaughterTrackResidualRange;
  std::vector<bool> fDaughterLongestTrack;
  double fT0;

  // Define input labels
  const std::string fPFParticleLabel;
  const std::string fTrackLabel;

  // Define input labels
  const std::string fCalorimetryLabel;

  // Define input for Slice
  const std::string fSliceLabel;
  const std::string fOptLabel;


  // Declare member data here.

};


test::AnalyzeEvents::AnalyzeEvents(fhicl::ParameterSet const& p)
  : EDAnalyzer{p},
  fPFParticleLabel(p.get<std::string>("PFParticleLabel")),
  fTrackLabel(p.get<std::string>("TrackLabel")),
  fCalorimetryLabel(p.get<std::string>("CalorimetryLabel")),
  fSliceLabel(p.get<std::string>("SliceLabel")),
  fOptLabel(p.get<std::string>("T0Label"))  // ,
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void test::AnalyzeEvents::analyze(art::Event const& e)
{
  // Implementation of required member function here.
  
  // Increment the event ID
  fEventID = e.id().event();

  // Set all counters to 0 for the current event
  fNPFParticles      = 0;
  fNPrimaries        = 0;
  fNPrimaryDaughters = 0;
  fDaughterTrackLengths.clear();
  fDaughterTrackdEdx.clear();
  fDaughterTrackResidualRange.clear();  
  fT0 = 0;

  // Search for the longest daughter ID
  int longestID        = -1;
  float longestLength  = std::numeric_limits<float>::lowest();

  // Load the PFParticles from Pandora
  art::Handle<std::vector<recob::PFParticle>> pfpHandle;
  std::vector<art::Ptr<recob::PFParticle>>    pfpVec;
  if(e.getByLabel(fPFParticleLabel, pfpHandle))
    art::fill_ptr_vector(pfpVec, pfpHandle);

  // Load the Tracks from Pandora
  art::Handle<std::vector<recob::Track>> trackHandle;
  std::vector<art::Ptr<recob::Track>>    trackVec;
  if(e.getByLabel(fTrackLabel, trackHandle))
    art::fill_ptr_vector(trackVec, trackHandle);

  // Load the Tracks from Pandora
  art::Handle<std::vector<recob::Slice>> sliceHandle;
  std::vector<art::Ptr<recob::Slice>>    sliceVec;
  if(e.getByLabel(fSliceLabel, sliceHandle))
    art::fill_ptr_vector(sliceVec, sliceHandle);


  // If there are no PFParticles then skip the event
  if(pfpVec.empty())
    return;

  // Initialise the neutrino ID
  size_t neutrinoID(std::numeric_limits<size_t>::max());

  // Loop over the PFParticles and find the neutrino
  for(const art::Ptr<recob::PFParticle> &pfp: pfpVec){
    fNPFParticles++;

    // Check that we are looking at a primary particle with a neutrino pdg code. If not, skip the PFParticle
    if(!(pfp->IsPrimary() && (std::abs(pfp->PdgCode()) == 14 || std::abs(pfp->PdgCode()) == 12)))
      continue;
    fNPrimaries++;

    neutrinoID         = pfp->Self();
    fNPrimaryDaughters = pfp->NumDaughters();
  } // PFParticle loop

  // Check that we found a neutrino
  if(neutrinoID == std::numeric_limits<size_t>::max())
    return; 

  // Get the tracks associated with the PFParticles
  art::FindManyP<recob::Track> pfpTrackAssns(pfpVec, e, fTrackLabel);
  art::FindManyP<anab::Calorimetry> trackCaloAssns(trackVec, e, fCalorimetryLabel);

  // Load the associations between PFPs, Slices and T0
  art::FindManyP<recob::Slice> pfpSliceAssns(pfpVec, e, fSliceLabel);
  art::FindManyP<anab::T0> sliceT0Assns(sliceVec, e, fOptLabel);


  for(const art::Ptr<recob::PFParticle> &pfp: pfpVec){
    // We are only interested in the neutrino daughter particles
    if(pfp->Parent() != neutrinoID)
      continue;

    // Get the tracks associated with this PFParticle
    const std::vector<art::Ptr<recob::Track>> pfpTracks(pfpTrackAssns.at(pfp.key()));

    // Get the slices associated with the current PFParticle
    const std::vector<art::Ptr<recob::Slice>> pfpSlices(pfpSliceAssns.at(pfp.key()));

    // There should only ever be 0 or 1 tracks associated with a single PFParticle
    if(pfpTracks.size() == 1){
      // Get the first element of the vector
      const art::Ptr<recob::Track> &pfpTrack(pfpTracks.front());

      // If this track is the longest, save the ID and set the length of the longest track
      if(pfpTrack->Length() > longestLength){
        longestID     = pfpTrack->ID();
        longestLength = pfpTrack->Length();

      } // Length check

      // Add parameters from the track to the branch vector
      fDaughterTrackLengths.push_back(pfpTrack->Length());
      fDaughterLongestTrack.push_back(pfpTrack->ID() == longestID);

      // Add parameters from the track to the branch vector
      fDaughterTrackLengths.push_back(pfpTrack->Length());

      // Fill the histogram with the track lengths
      fTrackLengthHist->Fill(pfpTrack->Length());
      
      // Now access the Calorimetry association for this track
      const std::vector<art::Ptr<anab::Calorimetry>> trackCalos(trackCaloAssns.at(pfpTrack.key()));

      // Now loop over the calorimetry objects and select the one on the collection plane
      for(const art::Ptr<anab::Calorimetry> &calo: trackCalos){
  
        // Get the plane number in a simple format
        const int planeNum(calo->PlaneID().Plane);

        // If it is not on the collection plane, skip it
        if(planeNum != 2)
          continue;

        // Add parameters from the calorimetry objects to the branch vector
        fDaughterTrackdEdx.push_back(calo->dEdx()); 
        fDaughterTrackResidualRange.push_back(calo->ResidualRange());
      } // Calorimetry Track association

    } // PFParticles Track

    // There should only ever be 0 or 1 slices associated to the neutrino PFP
    if (pfpSlices.size() == 1){
      // Get the first (only) element of the vector
      const art::Ptr<recob::Slice>& pfpSlice(pfpSlices.front());

      // Get the T0 object associated with the slice
      const std::vector<art::Ptr<anab::T0>> sliceT0s(sliceT0Assns.at(pfpSlice.key()));

      // There should only be 1 T0 per slice
      if (sliceT0s.size() == 1) {
        const art::Ptr<anab::T0>& t0(sliceT0s.front());
        fT0 = t0->Time();
      
      } // T0s

    } // Slices

  } // PFParticles


  // Store the outputs in the TTree
  fTree->Fill();
}

void test::AnalyzeEvents::beginJob()
{
  // Implementation of optional member function here.

  // Get the TFileService to create the output TTree for us
  art::ServiceHandle<art::TFileService> tfs;
  fTree = tfs->make<TTree>("tree", "Output TTree");
  fTrackLengthHist = tfs->make<TH1F>("trackLengthHist", "Reconstructed track lengths; Track length [cm]", 20, 0, 350);

  // Add branches to the TTree
  fTree->Branch("eventID", &fEventID);
  fTree->Branch("nPFParticles", &fNPFParticles);
  fTree->Branch("nPrimaries", &fNPrimaries);
  fTree->Branch("nPrimaryDaughters", &fNPrimaryDaughters);
  fTree->Branch("daughterTrackLengths", &fDaughterTrackLengths);
  fTree->Branch("daughterTrackdEdx", &fDaughterTrackdEdx);
  fTree->Branch("daughterTrackResidualRange", &fDaughterTrackResidualRange);
  fTree->Branch("daughterLongestTrack", &fDaughterLongestTrack);
  fTree->Branch("T0", &fT0);
  
}

void test::AnalyzeEvents::endJob()
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(test::AnalyzeEvents)
