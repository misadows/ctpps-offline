/****************************************************************************
 *
 * This is a part of TOTEM offline software.
 * Authors:
 *  Hubert Niewiadomski
 *  Jan Kašpar (jan.kaspar@gmail.com)
 *
 ****************************************************************************/

#include "TotemAnalysis/TotemNtuplizer/interface/RPNtuplizer.h"

#include "DataFormats/TotemRPDetId/interface/TotemRPDetId.h"

#include "DataFormats/CTPPSReco/interface/TotemRPCluster.h"
#include "DataFormats/CTPPSReco/interface/TotemRPUVPattern.h"
#include "DataFormats/CTPPSReco/interface/TotemRPLocalTrack.h"

#include "RecoTotemRP/RPRecoDataFormats/interface/RP2DHit.h"
#include "RecoTotemRP/RPRecoDataFormats/interface/RPReconstructedProton.h"
#include "RecoTotemRP/RPRecoDataFormats/interface/RPReconstructedProtonPairCollection.h"
#include "RecoTotemRP/RPRecoDataFormats/interface/RPReconstructedProtonCollection.h"
#include "RecoTotemRP/RPRecoDataFormats/interface/RPMulFittedTrackCollection.h"

#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/Common/interface/DetSet.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "RecoTotemRP/RPRecoDataFormats/interface/CentralMassInfo.h"

#include <iostream>
#include <string>

#include "TTree.h"

#include <map>

//----------------------------------------------------------------------------------------------------

ClassImp(RPRootDumpTrackInfo)
ClassImp(RPRootDumpDigiInfo)
ClassImp(RPRootDumpReconstructedProton)
ClassImp(RPRootDumpReconstructedProtonPair)

//----------------------------------------------------------------------------------------------------

using namespace std;
using namespace edm;

//----------------------------------------------------------------------------------------------------

RPNtuplizer::RPNtuplizer(const edm::ParameterSet& conf) :
  Ntuplizer(conf), Verbosity_(conf.getUntrackedParameter<unsigned int> ("verbosity", 0))
{
  modulLabelSimu_ = conf.getParameter<std::string> ("ModulLabelSimu");
  productLabelSimu_ = conf.getParameter<std::string> ("ProductLabelSimu");
  primaryProtons = false;
  rpFittedTrackCollectionLabel = conf.getParameter<edm::InputTag>("RPFittedTrackCollectionLabel");
  rpMulFittedTrackCollectionLabel = conf.getParameter<edm::InputTag>("RPMulFittedTrackCollectionLabel");
  rpStripDigiSetLabel = conf.getParameter<edm::InputTag>("TotemRPDigiSetLabel");
  rpDigClusterLabel = conf.getParameter<edm::InputTag>("RPDigClusterLabel");
  rpUVPatternLabel = conf.getParameter<edm::InputTag>("RPUVPatternLabel");
  rpReconstructedProtonCollectionLabel = conf.getParameter<edm::InputTag>("RPReconstructedProtonCollectionLabel");
  rpReconstructedProtonPairCollectionLabel = conf.getParameter<edm::InputTag>("RPReconstructedProtonPairCollectionLabel");

  if (conf.exists("primaryProtons"))
    primaryProtons = conf.getParameter<bool> ("primaryProtons");
  
  primaryJets_ = false;
  if (conf.exists("primaryJets"))
  { 
    primaryJets_ = conf.getParameter<bool> ("primaryJets");
    primaryJetsInstance_ = conf.getParameter<std::string> ("primaryJetsInstance");
    primaryJetsLabel_ = conf.getParameter<std::string> ("primaryJetsLabel");
  }

  includeDigi = false;
  if (conf.exists("includeDigi"))
    includeDigi = conf.getParameter<bool> ("includeDigi");
  
  includePatterns = false;
  if (conf.exists("includePatterns"))
    includePatterns = conf.getParameter<bool> ("includePatterns");
}

//----------------------------------------------------------------------------------------------------

void RPNtuplizer::DeclareConsumes(edm::EDAnalyzer *analyzer)
{
  // TODO: move here the statements from TotemNtuplizer::TotemNtuplizer
}

//----------------------------------------------------------------------------------------------------

void RPNtuplizer::CreateBranches(const edm::EventSetup &es, TTree *out_tree_)
{
  for (unsigned int a = 0; a < 2; ++a)
  {
    for (unsigned int s = 0; s < 3; ++s) 
    {
      if (s == 1)
        continue;

      for (unsigned int r = 0; r < 6; r++)
      {
        unsigned int id = 100 * a + 10 * s + r;
        char br_name[500];

        if (includeDigi)
        {
          sprintf(br_name, "digi_rp_%u.", id);
          out_tree_->Branch(br_name, &digi_info_[id]);
        }

        if (includePatterns)
        {
          sprintf(br_name, "patterns_rp_%u.", id);
          out_tree_->Branch(br_name, &patterns_info_[id]);
        }
        
        sprintf(br_name, "track_rp_%u.", id);
        out_tree_->Branch(br_name, &track_info_[id]);

        // TODO: uncomment
        //sprintf(br_name, "multi_track_rp_%u", id);
        //out_tree_->Branch(br_name, &multi_track_info_[id]);
      }
    }
  }

  // TODO: uncomment
  //out_tree_->Branch("rec_prot_left.", &rec_pr_info_[0]);
  //out_tree_->Branch("rec_prot_right.", &rec_pr_info_[1]);
  //out_tree_->Branch("rec_prot_pair.", &rec_pr_pair_info_);

  if (primaryProtons)
  {
    out_tree_->Branch("sim_prot_left.", &sim_pr_info_[0]);
    out_tree_->Branch("sim_prot_right.", &sim_pr_info_[1]);
  }
  
  if (primaryJets_)
  {
    out_tree_->Branch("MCjets.", &MCjets_);
    out_tree_->Branch("DiffMassInfo.", &diff_mass_info_);
  }
}

//----------------------------------------------------------------------------------------------------

void RPNtuplizer::FillEvent(const edm::Event& e, const edm::EventSetup& es)
{
#ifdef DEBUG
  printf(">> RPNtuplizer::FillEvent(%u:%u)\n", e.id().run(), e.id().event());
#endif

  // initialize objects with default values
  for (unsigned int a = 0; a < 2; ++a)
  {
    for (unsigned int s = 0; s < 3; ++s)
    {
      if (s == 1)
        continue;

      for (unsigned int r = 0; r < 6; r++)
      {
        unsigned int id = 100 * a + 10 * s + r;
        track_info_[id] = RPRootDumpTrackInfo();
  
        if (includeDigi)
          digi_info_[id] = RPRootDumpDigiInfo();

        if (includePatterns)
          patterns_info_[id].Reset();

        multi_track_info_[id].clear();
      }
    }
  }

  rec_pr_info_[0] = RPRootDumpReconstructedProton();
  rec_pr_info_[1] = RPRootDumpReconstructedProton();
  rec_pr_pair_info_ = RPRootDumpReconstructedProtonPair();
  
  if (primaryProtons)
  {
    sim_pr_info_[0] = RPRootDumpReconstructedProton();
    sim_pr_info_[1] = RPRootDumpReconstructedProton();
  }

  // single-RP track fits
  edm::Handle < DetSetVector<TotemRPLocalTrack> > fitted_tracks;
  e.getByLabel(rpFittedTrackCollectionLabel, fitted_tracks);

  if (fitted_tracks.isValid())
  {
    for (auto &ds : *fitted_tracks)
    {
      unsigned int rpId = ds.detId();

      if (ds.data.size() == 0)
        continue;

      if (ds.data.size() > 1)
        throw cms::Exception("RPNtuplizer::FillEvent") << ds.size() << " tracks is RP " << rpId << endl;

      const TotemRPLocalTrack &tr = ds[0];

      unsigned int entries = 0;
      for (auto &hds : tr.getHits())
        entries += hds.size();

      track_info_[rpId].valid = tr.isValid();
      track_info_[rpId].chi2 = tr.getChiSquared();
      track_info_[rpId].chi2ndf = tr.getChiSquaredOverNDF();
      track_info_[rpId].x = tr.getX0();
      track_info_[rpId].y = tr.getY0();
      track_info_[rpId].z = tr.getZ0();
      track_info_[rpId].thx = tr.getTx();
      track_info_[rpId].thy = tr.getTy();
      track_info_[rpId].entries = entries;
      track_info_[rpId].u_id = 0;
      track_info_[rpId].v_id = 0;
    }
  }

  // save the multi-track fits, if present
  // TODO: uncomment, update
  /*
  try {
    edm::Handle < RPMulFittedTrackCollection > multi_fitted_tracks;
    e.getByLabel(rpMulFittedTrackCollectionLabel, multi_fitted_tracks);

    if (multi_fitted_tracks.isValid())
    {
      for (RPMulFittedTrackCollection::const_iterator rit = multi_fitted_tracks->begin(); rit
              != multi_fitted_tracks->end(); ++rit)
      {
        std::vector < RPRootDumpTrackInfo > &tiv = multi_track_info_[rit->first];
      for (std::vector<RPFittedTrack>::const_iterator it = rit->second.begin(); it
                != rit->second.end(); ++it)
        {
          RPRootDumpTrackInfo ti;
        ti.valid = it->IsValid();
        ti.chi2 = it->ChiSquared();
        ti.chi2ndf = it->ChiSquaredOverN();
        ti.x = it->X0();
        ti.y = it->Y0();
        ti.z = it->Z0();
                  ti.thx = it->GetTx();
                  ti.thy = it->GetTy();
        ti.entries = it->GetHitEntries();
                  ti.u_id = it->GetUid();
                  ti.v_id = it->GetVid();
        tiv.push_back(ti);
      }
      }
    }
  } catch (const std::exception& e) { std::cout << "Exception: " << e.what() << std::endl;  }
  */

  if (includeDigi)
  {
    edm::Handle < edm::DetSetVector<TotemRPCluster> > clusters;
    e.getByLabel(rpDigClusterLabel, clusters);
  
    for (auto &ds : *clusters)
    {
      unsigned int detNo = TotemRPDetId::rawToDecId(ds.detId());
      unsigned int planeNo = detNo % 10;
      unsigned int RPNo = detNo / 10;
      
      for (auto &h : ds)
      {

        double centralStrip = h.getCenterStripPosition();
        int clusterSize = h.getNumberOfStrips();
  
        // if up to now no clusters in this plane -> increase no of planes counter
        if (digi_info_[RPNo].numberOfClusters[planeNo] == 0)
        {
          digi_info_[RPNo].numberOfPlanesOn++;
            if (TotemRPDetId::isStripsCoordinateUDirection(planeNo))
            digi_info_[RPNo].uPlanesOn++;
            else
            digi_info_[RPNo].vPlanesOn++;
        }
  
        digi_info_[RPNo].numberOfClusters[planeNo]++;
  
        digi_info_[RPNo].planeId.push_back(planeNo);
        digi_info_[RPNo].clusterSize.push_back(clusterSize);
        digi_info_[RPNo].centralStrip.push_back((int)centralStrip);
      }
    }
  }

  // fill in pattern-recognition results (non-parallel)
  if (includePatterns)
  {
    edm::Handle< DetSetVector<TotemRPUVPattern> > patterns;
    e.getByLabel(rpUVPatternLabel, patterns);
  
    for (auto &ds : *patterns)
    {
      unsigned int rp = ds.detId();
  
      for (auto &p : ds)
      {
        if (p.getProjection() == TotemRPUVPattern::projU)
        {
          patterns_info_[rp].u.push_back(RPRootDumpPattern(p.getA(), p.getB(), p.getW(), p.getFittable()));
          patterns_info_[rp].u_no = patterns_info_[rp].u.size(); 
        }
  
        if (p.getProjection() == TotemRPUVPattern::projV)
        {
          patterns_info_[rp].v.push_back(RPRootDumpPattern(p.getA(), p.getB(), p.getW(), p.getFittable()));
          patterns_info_[rp].v_no = patterns_info_[rp].v.size(); 
        }
      }
    }

    // TODO
    /*
    printf("---------------------------------------\n");
    for (auto &p: patterns_info_)
    {
      printf("%i: %i, %i\n", p.first, p.second.u_no, p.second.v_no);
    }
    */
  }

  // fill coincidence chip data
  /*
  edm::Handle < std::vector<TotemRPCCBits> > cc_chip_bits;
  e.getByLabel(modulLabelSimu_, productLabelSimu_, cc_chip_bits);
  if (cc_chip_bits.isValid())
  {
    std::vector<TotemRPCCBits>::const_iterator itc;
    for (itc = cc_chip_bits->begin(); itc != cc_chip_bits->end(); ++itc)
    {
      TotemRPDetId rawid(itc->getId());
      unsigned int decid = rawid.arm() * 100 + rawid.station() * 10 + rawid.romanPot();

      const auto &bits = itc->getBS();

      if (rawid.isStripsCoordinateUDirection())
      {
      for (int i = 0; i < 16; ++i)
        {
        if (bits[i])
          track_info_[decid].u_sect.push_back(i);
      }
      track_info_[decid].u_sect_no = track_info_[decid].u_sect.size();
      } else {
      for (int i = 0; i < 16; ++i)
        {
        if (bits[i])
          track_info_[decid].v_sect.push_back(i);
      }
      track_info_[decid].v_sect_no = track_info_[decid].v_sect.size();
      }
    }
  }
  */

  // fill in primary (simulated) proton data
  if (primaryProtons && FindSimulatedProtons(e) && FindSimulatedProtonsVertex(e))
  {
    if (left_prim_prot_.found)
      FindParametersOfSimulatedProtons(left_prim_prot_, 0);

    if (right_prim_prot_.found)
      FindParametersOfSimulatedProtons(right_prim_prot_, 1);
  }

  // fill in reconstructed proton data 
  if (FindReconstrucedProtons(e))
  {
    if (left_rec_prot_fund_)
    {
      rec_pr_info_[0].valid = reconstructed_protons_[0]->Valid();
      RPRecoProtMADXVariables mad_var = reconstructed_protons_[0]->GetMADXVariables();
  
      rec_pr_info_[0].thx = reconstructed_protons_[0]->Theta_x_angle();
      rec_pr_info_[0].thy = reconstructed_protons_[0]->Theta_y_angle();
      double reconstructed_phi = BOPar_.MADXCanonicalVariablesToCrossingAngleCorrectedPhi(mad_var);
      rec_pr_info_[0].phi = reconstructed_phi;
      double sin_phi = TMath::Sin(reconstructed_phi);
      double cos_phi = TMath::Cos(reconstructed_phi);
      double reconstructed_t = BOPar_.MADXCanonicalVariablesTot(mad_var);
  
      rec_pr_info_[0].t = reconstructed_t;
      rec_pr_info_[0].tx = reconstructed_t * cos_phi * cos_phi;
      rec_pr_info_[0].ty = reconstructed_t * sin_phi * sin_phi;
      rec_pr_info_[0].xi = reconstructed_protons_[0]->Ksi();
      rec_pr_info_[0].x0 = reconstructed_protons_[0]->X();
      rec_pr_info_[0].y0 = reconstructed_protons_[0]->Y();
      rec_pr_info_[0].chi2 = reconstructed_protons_[0]->Chi2();
      rec_pr_info_[0].chindf = reconstructed_protons_[0]->Chi2Norm();
    }

    if (right_rec_prot_fund_)
    {
      rec_pr_info_[1].valid = reconstructed_protons_[1]->Valid();
      RPRecoProtMADXVariables mad_var = reconstructed_protons_[1]->GetMADXVariables();

      rec_pr_info_[1].thx = reconstructed_protons_[1]->Theta_x_angle();
      rec_pr_info_[1].thy = reconstructed_protons_[1]->Theta_y_angle();
      double reconstructed_phi = BOPar_.MADXCanonicalVariablesToCrossingAngleCorrectedPhi(mad_var);
      rec_pr_info_[1].phi = reconstructed_phi;
      double sin_phi = TMath::Sin(reconstructed_phi);
      double cos_phi = TMath::Cos(reconstructed_phi);
      double reconstructed_t = BOPar_.MADXCanonicalVariablesTot(mad_var);
      //      std::cout<<"reconstructed_t="<<reconstructed_t<<"  rec_pr_info_[1].thx"<<rec_pr_info_[1].thx<<std::endl;

      rec_pr_info_[1].t = reconstructed_t;
      rec_pr_info_[1].tx = reconstructed_t * cos_phi * cos_phi;
      rec_pr_info_[1].ty = reconstructed_t * sin_phi * sin_phi;
      rec_pr_info_[1].xi = reconstructed_protons_[1]->Ksi();
      rec_pr_info_[1].x0 = reconstructed_protons_[1]->X();
      rec_pr_info_[1].y0 = reconstructed_protons_[1]->Y();
      rec_pr_info_[1].chi2 = reconstructed_protons_[1]->Chi2();
      rec_pr_info_[1].chindf = reconstructed_protons_[1]->Chi2Norm();
    }
  }

  if (FindReconstrucedProtonPair(e))
  {
    rec_pr_pair_info_.valid = reconstructed_proton_pair_.Valid();
    rec_pr_pair_info_.x0 = reconstructed_proton_pair_.X3D();
    rec_pr_pair_info_.y0 = reconstructed_proton_pair_.Y3D();
    rec_pr_pair_info_.z0 = reconstructed_proton_pair_.Z3D();
    rec_pr_pair_info_.thxl = reconstructed_proton_pair_.ThetaXAngleLeft();
    rec_pr_pair_info_.thyl = reconstructed_proton_pair_.ThetaYAngleLeft();
    rec_pr_pair_info_.thxr = reconstructed_proton_pair_.ThetaXAngleRight();
    rec_pr_pair_info_.thyr = reconstructed_proton_pair_.ThetaYAngleRight();
    rec_pr_pair_info_.xil = reconstructed_proton_pair_.KsiLeft();
    rec_pr_pair_info_.xir = reconstructed_proton_pair_.KsiRight();
    rec_pr_pair_info_.chi2 = reconstructed_proton_pair_.Chi2();
    rec_pr_pair_info_.chindf = reconstructed_proton_pair_.Chi2Norm();

    RPRecoProtMADXVariables mad_var_right = reconstructed_proton_pair_.GetMADXVariablesRight();
    RPRecoProtMADXVariables mad_var_left = reconstructed_proton_pair_.GetMADXVariablesLeft();

    double phi_reconst_right = BOPar_.MADXCanonicalVariablesToCrossingAngleCorrectedPhi(mad_var_right);
    double phi_reconst_left = BOPar_.MADXCanonicalVariablesToCrossingAngleCorrectedPhi(mad_var_left);
    double cos_rec_phir = TMath::Cos(phi_reconst_right);
    double sin_rec_phir = TMath::Sin(phi_reconst_right);
    double cos_rec_phil = TMath::Cos(phi_reconst_left);
    double sin_rec_phil = TMath::Sin(phi_reconst_left);

    rec_pr_pair_info_.phir = phi_reconst_right;
    rec_pr_pair_info_.phil = phi_reconst_left;

    double t_reconst_right = BOPar_.MADXCanonicalVariablesTot(mad_var_right);
    double t_reconst_left = BOPar_.MADXCanonicalVariablesTot(mad_var_left);

    rec_pr_pair_info_.tr = t_reconst_right;
    rec_pr_pair_info_.tl = t_reconst_left;

    rec_pr_pair_info_.txr = t_reconst_right * cos_rec_phir * cos_rec_phir;
    rec_pr_pair_info_.tyr = t_reconst_right * sin_rec_phir * sin_rec_phir;
    rec_pr_pair_info_.txl = t_reconst_left * cos_rec_phil * cos_rec_phil;
    rec_pr_pair_info_.tyl = t_reconst_left * sin_rec_phil * sin_rec_phil;

    rec_pr_pair_info_.t = BOPar_.MADXCanonicalVariablesToElasticPhysics_t(mad_var_right, mad_var_left);
  }
  
  // fill in other primary (simulated) object data
  if(primaryJets_)
  {
    edm::Handle < std::vector<fastjet::PseudoJet> > MCjets_handle;
    e.getByLabel(primaryJetsInstance_, primaryJetsLabel_, MCjets_handle);
    if(MCjets_handle.isValid())
    {
      if (Verbosity_)
        std::cout << "MC jets found" << std::endl;
      MCjets_.AddJets( (*MCjets_handle) );
    }
    else
    {
      if (Verbosity_)
        std::cout << "MC jets not found" << std::endl;
    }
    
    edm::Handle < CentralMassInfo > central_mass_info_handle;
    e.getByLabel(primaryJetsInstance_, primaryJetsLabel_, central_mass_info_handle);
    if(central_mass_info_handle.isValid())
    {
      if (Verbosity_)
        std::cout << "Central mass info found" << std::endl;
      diff_mass_info_.SetVariables( (*central_mass_info_handle) );
    }
    else
    {
      if (Verbosity_)
        std::cout << "Central mass info not found" << std::endl;
    }
  }
}

//----------------------------------------------------------------------------------------------------

void RPNtuplizer::FindParametersOfSimulatedProtons(PrimaryProton proton, int simulatedProtonNumber)
{
  double phi = BOPar_.ComputeNonSmearedProtonPhi(proton.momentum);
  double sin_phi = TMath::Sin(phi);
  double cos_phi = TMath::Cos(phi);
  sim_pr_info_[simulatedProtonNumber].phi = phi;
  sim_pr_info_[simulatedProtonNumber].x0 = proton.vertex.x();
  sim_pr_info_[simulatedProtonNumber].y0 = proton.vertex.y();
  sim_pr_info_[simulatedProtonNumber].thx = BOPar_.IPNonSmearedProtonMomentumToThetaX(proton.momentum);
  sim_pr_info_[simulatedProtonNumber].thy = BOPar_.IPNonSmearedProtonMomentumToThetaY(proton.momentum);
  sim_pr_info_[simulatedProtonNumber].xi = BOPar_.IPNonSmearedProtonMomentumToXi(proton.momentum.px(),
          proton.momentum.py(), proton.momentum.pz());
  sim_pr_info_[simulatedProtonNumber].t = BOPar_.IPNonSmearedProtonMomentumTot(proton.momentum.px(),
          proton.momentum.py(), proton.momentum.pz());
  sim_pr_info_[simulatedProtonNumber].tx = sim_pr_info_[simulatedProtonNumber].t * cos_phi * cos_phi;
  sim_pr_info_[simulatedProtonNumber].ty = sim_pr_info_[simulatedProtonNumber].t * sin_phi * sin_phi;
  sim_pr_info_[simulatedProtonNumber].valid = true;
}

//----------------------------------------------------------------------------------------------------

bool RPNtuplizer::FindReconstrucedProtons(const edm::Event& e)
{
  reconstructed_protons_.clear();

  if (Verbosity_)
    std::cout << "Finding the reconstructed protons" << std::endl;
  edm::Handle < RPReconstructedProtonCollection > input;
  e.getByLabel(rpReconstructedProtonCollectionLabel, input);

  right_rec_prot_fund_ = 0;
  left_rec_prot_fund_ = 0;
  if (input.isValid()) {
    for (RPReconstructedProtonCollection::const_iterator it = input->begin(); it != input->end(); ++it) {
      if (Verbosity_)
        std::cout << "Reconstructed proton ksi" << it->Ksi() << "zdirection:" << it->ZDirection()
                << std::endl;
      if (it->ZDirection() > 0) {
        ++right_rec_prot_fund_;
        reconstructed_protons_[1] = &(*it);
        if (Verbosity_)
          std::cout << "Setting the reconstructed proton right" << std::endl;
      }
      if (it->ZDirection() < 0) {
        ++left_rec_prot_fund_;
        reconstructed_protons_[0] = &(*it);
        if (Verbosity_)
          std::cout << "Setting the reconstructed proton left" << std::endl;
      }
    }
  }
  bool result = right_rec_prot_fund_ <= 1 && left_rec_prot_fund_ <= 1 && (right_rec_prot_fund_ > 0
          || left_rec_prot_fund_ > 0);
  return result;
}

//----------------------------------------------------------------------------------------------------

bool RPNtuplizer::FindSimulatedProtons(const edm::Event& e)
{
  edm::Handle < edm::HepMCProduct > input;
  e.getByLabel("SmearingGenerator", "original", input);

  if (input.isValid())
  {
    const HepMC::GenEvent *evt = input->GetEvent();
    right_prim_prot_.found = false;
    left_prim_prot_.found = false;

    for (HepMC::GenEvent::particle_const_iterator it = evt->particles_begin(); it != evt->particles_end(); ++it)
    {
      HepMC::GenParticle * g = (*it);
      int g_status = g->status();
      int pdg_id = g->pdg_id();

      // scanning only for particles with status == 1
      if (g_status == 1 && pdg_id == 2212)
      {
        const HepMC::FourVector &vtx = g->production_vertex()->position();
        const HepMC::FourVector &mom = g->momentum();

        if (mom.z() > 0)
        {
          if (!right_prim_prot_.found || ( fabs(mom.z()) > fabs(right_prim_prot_.momentum.z()) ) )
          {
            right_prim_prot_.vertex = vtx; //[mm]
            right_prim_prot_.momentum = mom; //[GeV]
            right_prim_prot_.found = true;
          }
        }

        if (mom.z() < 0)
        {
          if (!left_prim_prot_.found || (fabs(mom.z()) > fabs(left_prim_prot_.momentum.z())) )
          {
            left_prim_prot_.vertex = vtx; //[mm]
            left_prim_prot_.momentum = mom; //[GeV]
            left_prim_prot_.found = true;
          }
        }
      }
    }

    return true;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------

bool RPNtuplizer::FindSimulatedProtonsVertex(const edm::Event& e)
{
  edm::Handle < edm::HepMCProduct > input;
  e.getByLabel("generator", input);
  
  bool left_found = false;
  bool right_found = false;
  HepMC::FourVector mom_left;
  HepMC::FourVector mom_right;
  
  if (input.isValid()) {
    const HepMC::GenEvent *evt = input->GetEvent();

    for (HepMC::GenEvent::particle_const_iterator it = evt->particles_begin(); it != evt->particles_end(); ++it) {
      HepMC::GenParticle * g = (*it);
      int g_status = g->status();
      int pdg_id = g->pdg_id();

      // scanning only for particles with status == 1
      if (g_status == 1 && pdg_id == 2212) {
        const HepMC::FourVector &vtx = g->production_vertex()->position();
        const HepMC::FourVector &mom = g->momentum();

        if (mom.z() > 0) {
          if (!right_found || (fabs(mom.z()) > fabs(mom_right.z())) ) {
            right_prim_prot_.vertex = vtx; //[mm]
            mom_right = mom; //[GeV]
            right_found = true;
          }
        }
        if (mom.z() < 0) {
          if (!left_found || (fabs(mom.z()) > fabs(mom_left.z())) ) {
            left_prim_prot_.vertex = vtx; //[mm]
            mom_left = mom; //[GeV]
            left_found = true;
          }
        }
      }
    }
    
//    if(!result)
//      std::cout<<"FindSimulatedProtonsVertex:: Vertex not found, 1"<<std::endl;
//    else
//      std::cout<<"FindSimulatedProtonsVertex:: Vertex found"<<std::endl; 

    return true;
  }
  
//  std::cout<<"FindSimulatedProtonsVertex:: Vertex not found, 2"<<std::endl;
  return false;
}

//----------------------------------------------------------------------------------------------------

bool RPNtuplizer::FindReconstrucedProtonPair(const edm::Event& e)
{
  if (Verbosity_)
    std::cout << "Finding the reconstructed protons" << std::endl;
  edm::Handle < RPReconstructedProtonPairCollection > input;
  e.getByLabel(rpReconstructedProtonPairCollectionLabel, input);

  if (!input.isValid() || input->size() != 1) {
    if (Verbosity_)
      std::cout << "reconstructed protons not found" << std::endl;
    return false;
  } else {
    if (Verbosity_)
      std::cout << "reconstructed protons found" << std::endl;
    reconstructed_proton_pair_ = (*input)[0];
    return true;
  }
}
