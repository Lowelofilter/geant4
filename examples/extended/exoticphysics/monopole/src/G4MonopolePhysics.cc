//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file exoticphysics/monopole/src/G4MonopolePhysics.cc
/// \brief Implementation of the G4MonopolePhysics class
//
// $Id: G4MonopolePhysics.cc 66994 2013-01-29 14:34:08Z gcosmo $
//
//---------------------------------------------------------------------------
//
// ClassName:   G4MonopolePhysics
//
// Author:      V.Ivanchenko 13.03.2005
//
// Modified:
//
//  12.07.10  S.Burdin (changed the magnetic and electric charge variables from integer to double)
//----------------------------------------------------------------------------
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "G4MonopolePhysics.hh"
#include "G4MonopolePhysicsMessenger.hh"

#include "G4Monopole.hh"
#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"

#include "G4StepLimiter.hh"
#include "G4Transportation.hh"
#include "G4MonopoleTransportation.hh"
#include "G4hMultipleScattering.hh"
#include "G4mplIonisation.hh"
#include "G4mplIonisationWithDeltaModel.hh"
#include "G4hhIonisation.hh"
#include "G4hIonisation.hh"

#include "G4PhysicsListHelper.hh"
#include "G4BuilderType.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4MonopolePhysics::G4MonopolePhysics(const G4String& nam)
  : G4VPhysicsConstructor(nam)
{
  fMagCharge = 1.0;
  //  fMagCharge = -1.0;
  //  fElCharge  = -50.0;
  fElCharge  = 0.0;
  fMonopoleMass = 100.*GeV;
  fMessenger = new G4MonopolePhysicsMessenger(this);
  fMpl = 0;
  SetPhysicsType(bUnknown);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4MonopolePhysics::~G4MonopolePhysics()
{
  delete fMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4MonopolePhysics::ConstructParticle()
{
  fMpl = G4Monopole::MonopoleDefinition(fMonopoleMass, fMagCharge, fElCharge);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4MonopolePhysics::ConstructProcess()
{
  if(verboseLevel > 0) {
    G4cout << "G4MonopolePhysics::ConstructProcess" << G4endl;
  }
  
  G4PhysicsListHelper* ph = G4PhysicsListHelper::GetPhysicsListHelper();
  G4ProcessManager* pmanager = new G4ProcessManager(fMpl);
  fMpl->SetProcessManager(pmanager);
  
  // defined monopole parameters and binning

  G4double magn = fMpl->MagneticCharge();
  G4double emin = fMonopoleMass/20000.;
  if(emin < keV) { emin = keV; }
  G4double emax = std::max(10.*TeV, fMonopoleMass*100);
  G4int nbin = G4lrint(10*std::log10(emax/emin));

  if(magn == 0.0) {
    ph->RegisterProcess(new G4Transportation(), fMpl);
  } else {
    ph->RegisterProcess(new G4MonopoleTransportation(fMpl), fMpl);
  }

  if(fMpl->GetPDGCharge() != 0.0) {
    G4hIonisation* hhioni = new G4hIonisation();
    hhioni->SetDEDXBinning(nbin);
    hhioni->SetMinKinEnergy(emin);
    hhioni->SetMaxKinEnergy(emax);
    ph->RegisterProcess(hhioni, fMpl);
  }
  if(magn != 0.0) {
    G4mplIonisation* mplioni = new G4mplIonisation(magn);
    mplioni->SetDEDXBinning(nbin);
    mplioni->SetMinKinEnergy(emin);
    mplioni->SetMaxKinEnergy(emax);
    ph->RegisterProcess(mplioni, fMpl);
  }
  ph->RegisterProcess(new G4StepLimiter(), fMpl);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4MonopolePhysics::SetMagneticCharge(G4double val)
{
  fMagCharge = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4MonopolePhysics::SetElectricCharge(G4double val)
{
  fElCharge = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4MonopolePhysics::SetMonopoleMass(G4double mass)
{
  fMonopoleMass = mass;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

