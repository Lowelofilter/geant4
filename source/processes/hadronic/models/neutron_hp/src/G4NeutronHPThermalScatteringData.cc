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
// Thermal Neutron Scattering
// Koi, Tatsumi (SCCS/SLAC)
//
// Class Description
// Cross Sections for a high precision (based on evaluated data
// libraries) description of themal neutron scattering below 4 eV;
// Based on Thermal neutron scattering files
// from the evaluated nuclear data files ENDF/B-VI, Release2
// To be used in your physics list in case you need this physics.
// In this case you want to register an object of this class with
// the corresponding process.
// Class Description - End

// 15-Nov-06 First implementation is done by T. Koi (SLAC/SCCS)
// 070625 implement clearCurrentXSData to fix memory leaking by T. Koi

#include <list>
#include <algorithm>

#include "G4NeutronHPThermalScatteringData.hh"
#include "G4SystemOfUnits.hh"
#include "G4Neutron.hh"
#include "G4ElementTable.hh"
//#include "G4NeutronHPData.hh"

G4NeutronHPThermalScatteringData::G4NeutronHPThermalScatteringData()
:G4VCrossSectionDataSet("NeutronHPThermalScatteringData")
{
// Upper limit of neutron energy 
   emax = 4*eV;
   SetMinKinEnergy( 0*MeV );                                   
   SetMaxKinEnergy( emax );                                   

   ke_cache = 0.0;
   xs_cache = 0.0;
   element_cache = NULL;
   material_cache = NULL;

   indexOfThermalElement.clear(); 

   names = new G4NeutronHPThermalScatteringNames();

   //BuildPhysicsTable( *G4Neutron::Neutron() );
}

G4NeutronHPThermalScatteringData::~G4NeutronHPThermalScatteringData()
{

   clearCurrentXSData();

   delete names;
}

G4bool G4NeutronHPThermalScatteringData::IsIsoApplicable( const G4DynamicParticle* dp , 
                                                G4int /*Z*/ , G4int /*A*/ ,
                                                const G4Element* element ,
                                                const G4Material* material )
{
   G4double eKin = dp->GetKineticEnergy();
   if ( eKin > 4.0*eV //GetMaxKinEnergy() 
     || eKin < 0 //GetMinKinEnergy() 
     || dp->GetDefinition() != G4Neutron::Neutron() ) return false;                                   

   if ( dic.find( std::pair < const G4Material* , const G4Element* > ( (G4Material*)NULL , element ) ) != dic.end() 
     || dic.find( std::pair < const G4Material* , const G4Element* > ( material , element ) ) != dic.end() ) return true;

   return false;

//   return IsApplicable( dp , element );
/*
   G4double eKin = dp->GetKineticEnergy();
   if ( eKin > 4.0*eV //GetMaxKinEnergy() 
     || eKin < 0 //GetMinKinEnergy() 
     || dp->GetDefinition() != G4Neutron::Neutron() ) return false;                                   
   return true;
*/
}

G4double G4NeutronHPThermalScatteringData::GetIsoCrossSection( const G4DynamicParticle* dp ,
                                   G4int /*Z*/ , G4int /*A*/ ,
                                   const G4Isotope* /*iso*/  ,
                                   const G4Element* element ,
                                   const G4Material* material )
{
   if ( dp->GetKineticEnergy() == ke_cache && element == element_cache &&  material == material_cache ) return xs_cache;

   ke_cache = dp->GetKineticEnergy();
   element_cache = element;
   material_cache = material;
   //G4double xs = GetCrossSection( dp , element , material->GetTemperature() );
   G4double xs = GetCrossSection( dp , element , material );
   xs_cache = xs;
   return xs;
   //return GetCrossSection( dp , element , material->GetTemperature() );
}

void G4NeutronHPThermalScatteringData::clearCurrentXSData()
{
   std::map< G4int , std::map< G4double , G4NeutronHPVector* >* >::iterator it;
   std::map< G4double , G4NeutronHPVector* >::iterator itt;

   for ( it = coherent.begin() ; it != coherent.end() ; it++ )
   {
      if ( it->second != NULL )
      {
         for ( itt = it->second->begin() ; itt != it->second->end() ; itt++ )
         {
            delete itt->second;
         }
      }
      delete it->second;
   }

   for ( it = incoherent.begin() ; it != incoherent.end() ; it++ )
   {
      if ( it->second != NULL )
      { 
         for ( itt = it->second->begin() ; itt != it->second->end() ; itt++ )
         {
            delete itt->second;
         }
      }
      delete it->second;
   }

   for ( it = inelastic.begin() ; it != inelastic.end() ; it++ )
   {
      if ( it->second != NULL )
      {
         for ( itt = it->second->begin() ; itt != it->second->end() ; itt++ )
         {
            delete itt->second;
         }
      }
      delete it->second; 
   }

   coherent.clear();
   incoherent.clear();
   inelastic.clear();

}



G4bool G4NeutronHPThermalScatteringData::IsApplicable( const G4DynamicParticle* aP , const G4Element* anEle )
{
   G4bool result = false;

   G4double eKin = aP->GetKineticEnergy();
   // Check energy 
   if ( eKin < emax )
   {
      // Check Particle Species
      if ( aP->GetDefinition() == G4Neutron::Neutron() ) 
      {
        // anEle is one of Thermal elements 
         G4int ie = (G4int) anEle->GetIndex();
         std::vector < G4int >::iterator it; 
         for ( it = indexOfThermalElement.begin() ; it != indexOfThermalElement.end() ; it++ )
         {
             if ( ie == *it ) return true;
         }
      }
   }

/*
   if ( names->IsThisThermalElement ( anEle->GetName() ) )
   {
      // Check energy and projectile species 
      G4double eKin = aP->GetKineticEnergy();
      if ( eKin < emax && aP->GetDefinition() == G4Neutron::Neutron() ) result = true; 
   }
*/
   return result;
}


void G4NeutronHPThermalScatteringData::BuildPhysicsTable(const G4ParticleDefinition& aP)
{

   if ( &aP != G4Neutron::Neutron() ) 
      throw G4HadronicException(__FILE__, __LINE__, "Attempt to use NeutronHP data for particles other than neutrons!!!");  

   //std::map < std::pair < G4Material* , const G4Element* > , G4int > dic;   
   dic.clear();   
   clearCurrentXSData();
   std::map < G4String , G4int > co_dic;   

   //Searching Nist Materials
   static const G4MaterialTable* theMaterialTable = G4Material::GetMaterialTable();
   size_t numberOfMaterials = G4Material::GetNumberOfMaterials();
   for ( size_t i = 0 ; i < numberOfMaterials ; i++ )
   {
      G4Material* material = (*theMaterialTable)[i];
      size_t numberOfElements = material->GetNumberOfElements();
      for ( size_t j = 0 ; j < numberOfElements ; j++ )
      {
         const G4Element* element = material->GetElement(j);
         if ( names->IsThisThermalElement ( material->GetName() , element->GetName() ) )
         {                                    
            G4int ts_ID_of_this_geometry; 
            G4String ts_ndl_name = names->GetTS_NDL_Name( material->GetName() , element->GetName() ); 
            if ( co_dic.find ( ts_ndl_name ) != co_dic.end() )
            {
               ts_ID_of_this_geometry = co_dic.find ( ts_ndl_name ) -> second;
            }
            else
            {
               ts_ID_of_this_geometry = co_dic.size();
               co_dic.insert ( std::pair< G4String , G4int >( ts_ndl_name , ts_ID_of_this_geometry ) );
            }

            //G4cout << "Neutron HP Thermal Scattering Data : Registering a material-element pair of " 
            //       << material->GetName() << " " << element->GetName() 
            //       << " as internal thermal scattering id of  " <<  ts_ID_of_this_geometry << "." << G4endl;

            dic.insert( std::pair < std::pair < G4Material* , const G4Element* > , G4int > ( std::pair < G4Material* , const G4Element* > ( material , element ) , ts_ID_of_this_geometry ) );
         }
      }
   }

   //Searching TS Elements 
   static const G4ElementTable* theElementTable = G4Element::GetElementTable();
   size_t numberOfElements = G4Element::GetNumberOfElements();
   //size_t numberOfThermalElements = 0; 
   for ( size_t i = 0 ; i < numberOfElements ; i++ )
   {
      const G4Element* element = (*theElementTable)[i];
      if ( names->IsThisThermalElement ( element->GetName() ) )
      {
         if ( names->IsThisThermalElement ( element->GetName() ) )
         {                                    
            G4int ts_ID_of_this_geometry; 
            G4String ts_ndl_name = names->GetTS_NDL_Name( element->GetName() ); 
            if ( co_dic.find ( ts_ndl_name ) != co_dic.end() )
            {
               ts_ID_of_this_geometry = co_dic.find ( ts_ndl_name ) -> second;
            }
            else
            {
               ts_ID_of_this_geometry = co_dic.size();
               co_dic.insert ( std::pair< G4String , G4int >( ts_ndl_name , ts_ID_of_this_geometry ) );
            }

            //G4cout << "Neutron HP Thermal Scattering: Registering an element of " 
            //       << material->GetName() << " " << element->GetName() 
            //       << " as internal thermal scattering id of  " <<  ts_ID_of_this_geometry << "." << G4endl;

            dic.insert( std::pair < std::pair < const G4Material* , const G4Element* > , G4int > ( std::pair < const G4Material* , const G4Element* > ( (G4Material*)NULL , element ) ,  ts_ID_of_this_geometry ) );
         }
      }
   }

   G4cout << G4endl;
   G4cout << "Neutron HP Thermal Scattering Data: Following material-element pairs and/or elements are registered." << G4endl;
   for ( std::map < std::pair < const G4Material* , const G4Element* > , G4int >::iterator it = dic.begin() ; it != dic.end() ; it++ )   
   {
      if ( it->first.first != NULL ) 
      {
         G4cout << "Material " << it->first.first->GetName() << " - Element " << it->first.second->GetName() << ",  internal thermal scattering id " << it->second << G4endl;
      }
      else
      {
         G4cout << "Element " << it->first.second->GetName() << ",  internal thermal scattering id " << it->second << G4endl;
      }
   }
   G4cout << G4endl;


   //G4cout << "Neutron HP Thermal Scattering Data: Following NDL thermal scattering files are assigned to the internal thermal scattering id." << G4endl;
   //for ( std::map < G4String , G4int >::iterator it = co_dic.begin() ; it != co_dic.end() ; it++ )  
   //{
   //   G4cout << "NDL file name " << it->first << ", internal thermal scattering id " << it->second << G4endl;
   //}


   // Read Cross Section Data files

   G4String dirName;
   if ( !getenv( "G4NEUTRONHPDATA" ) ) 
      throw G4HadronicException(__FILE__, __LINE__, "Please setenv G4NEUTRONHPDATA to point to the neutron cross-section files.");
   G4String baseName = getenv( "G4NEUTRONHPDATA" );

   dirName = baseName + "/ThermalScattering";

   G4String ndl_filename;
   G4String full_name;

   for ( std::map < G4String , G4int >::iterator it = co_dic.begin() ; it != co_dic.end() ; it++ )  
   {
      ndl_filename = it->first;
      G4int ts_ID = it->second;

      // Coherent
      full_name = dirName + "/Coherent/CrossSection/" + ndl_filename; 
      std::map< G4double , G4NeutronHPVector* >*  coh_amapTemp_EnergyCross = readData( full_name );
      coherent.insert ( std::pair < G4int , std::map< G4double , G4NeutronHPVector* >* > ( ts_ID , coh_amapTemp_EnergyCross ) );

      // Incoherent
      full_name = dirName + "/Incoherent/CrossSection/" + ndl_filename; 
      std::map< G4double , G4NeutronHPVector* >*  incoh_amapTemp_EnergyCross = readData( full_name );
      incoherent.insert ( std::pair < G4int , std::map< G4double , G4NeutronHPVector* >* > ( ts_ID , incoh_amapTemp_EnergyCross ) );

      // Inelastic
      full_name = dirName + "/Inelastic/CrossSection/" + ndl_filename; 
      std::map< G4double , G4NeutronHPVector* >*  inela_amapTemp_EnergyCross = readData( full_name );
      inelastic.insert ( std::pair < G4int , std::map< G4double , G4NeutronHPVector* >* > ( ts_ID , inela_amapTemp_EnergyCross ) );

   }

}



std::map< G4double , G4NeutronHPVector* >* G4NeutronHPThermalScatteringData::readData ( G4String full_name ) 
{

   std::map< G4double , G4NeutronHPVector* >*  aData = new std::map< G4double , G4NeutronHPVector* >; 
   
   std::ifstream theChannel( full_name.c_str() );

   //G4cout << "G4NeutronHPThermalScatteringData " << name << G4endl;

   G4int dummy; 
   while ( theChannel >> dummy )   // MF
   {
      theChannel >> dummy;   // MT
      G4double temp; 
      theChannel >> temp;   
      G4NeutronHPVector* anEnergyCross = new G4NeutronHPVector;
      G4int nData;
      theChannel >> nData;
      anEnergyCross->Init ( theChannel , nData , eV , barn );
      aData->insert ( std::pair < G4double , G4NeutronHPVector* > ( temp , anEnergyCross ) );
   }
   theChannel.close();

   return aData;

} 



void G4NeutronHPThermalScatteringData::DumpPhysicsTable( const G4ParticleDefinition& aP )
{
   if( &aP != G4Neutron::Neutron() ) 
     throw G4HadronicException(__FILE__, __LINE__, "Attempt to use NeutronHP data for particles other than neutrons!!!");  
//  G4cout << "G4NeutronHPThermalScatteringData::DumpPhysicsTable still to be implemented"<<G4endl;
}

//#include "G4Nucleus.hh"
//#include "G4NucleiPropertiesTable.hh"
//#include "G4Neutron.hh"
//#include "G4Electron.hh"



/*
G4double G4NeutronHPThermalScatteringData::GetCrossSection( const G4DynamicParticle* aP , const G4Element*anE , G4double aT )
{

   G4double result = 0;
   const G4Material* aM = NULL;

   G4int iele = anE->GetIndex();

   if ( dic.find( std::pair < const G4Material* , const G4Element* > ( (G4Material*)NULL , anE ) ) != dic.end() )
   {
      iele = dic.find( std::pair < const G4Material* , const G4Element* > ( (G4Material*)NULL , anE ) )->second;
   }
   else if ( dic.find( std::pair < const G4Material* , const G4Element* > ( aM , anE ) ) != dic.end() )
   {
      iele = dic.find( std::pair < const G4Material* , const G4Element* > ( aM , anE ) )->second;
   }
   else
   {
      return result;
   }

   G4double Xcoh = GetX ( aP , aT , coherent.find(iele)->second );
   G4double Xincoh = GetX ( aP , aT , incoherent.find(iele)->second );
   G4double Xinela = GetX ( aP , aT , inelastic.find(iele)->second );

   result = Xcoh + Xincoh + Xinela;

   //G4cout << "G4NeutronHPThermalScatteringData::GetCrossSection  Tot= " << result/barn << " Coherent= " << Xcoh/barn << " Incoherent= " << Xincoh/barn << " Inelastic= " << Xinela/barn << G4endl;

   return result;

}
*/

G4double G4NeutronHPThermalScatteringData::GetCrossSection( const G4DynamicParticle* aP , const G4Element*anE , const G4Material* aM )
{
   G4double result = 0;
   
   G4int ts_id =getTS_ID( aM , anE );

   if ( ts_id == -1 ) return result;

   G4double aT = aM->GetTemperature();

   G4double Xcoh = GetX ( aP , aT , coherent.find(ts_id)->second );
   G4double Xincoh = GetX ( aP , aT , incoherent.find(ts_id)->second );
   G4double Xinela = GetX ( aP , aT , inelastic.find(ts_id)->second );

   result = Xcoh + Xincoh + Xinela;

   //G4cout << "G4NeutronHPThermalScatteringData::GetCrossSection  Tot= " << result/barn << " Coherent= " << Xcoh/barn << " Incoherent= " << Xincoh/barn << " Inelastic= " << Xinela/barn << G4endl;

   return result;
}


G4double G4NeutronHPThermalScatteringData::GetInelasticCrossSection( const G4DynamicParticle* aP , const G4Element*anE , const G4Material* aM )
{
   G4double result = 0;
   G4int ts_id = getTS_ID( aM , anE );
   G4double aT = aM->GetTemperature();
   result = GetX ( aP , aT , inelastic.find( ts_id )->second );
   return result;
}

G4double G4NeutronHPThermalScatteringData::GetCoherentCrossSection( const G4DynamicParticle* aP , const G4Element*anE , const G4Material* aM )
{
   G4double result = 0;
   G4int ts_id = getTS_ID( aM , anE );
   G4double aT = aM->GetTemperature();
   result = GetX ( aP , aT , coherent.find( ts_id )->second );
   return result;
}

G4double G4NeutronHPThermalScatteringData::GetIncoherentCrossSection( const G4DynamicParticle* aP , const G4Element*anE , const G4Material* aM )
{
   G4double result = 0;
   G4int ts_id = getTS_ID( aM , anE );
   G4double aT = aM->GetTemperature();
   result = GetX ( aP , aT , incoherent.find( ts_id )->second );
   return result;
}



G4int G4NeutronHPThermalScatteringData::getTS_ID ( const G4Material* material , const G4Element* element )
{
   G4int result = -1;
   if ( dic.find( std::pair < const G4Material* , const G4Element* > ( (G4Material*)NULL , element ) ) != dic.end() ) 
      return dic.find( std::pair < const G4Material* , const G4Element* > ( (G4Material*)NULL , element ) )->second; 
   if ( dic.find( std::pair < const G4Material* , const G4Element* > ( material , element ) ) != dic.end() ) 
      return dic.find( std::pair < const G4Material* , const G4Element* > ( material , element ) )->second; 
   return result; 
}




G4double G4NeutronHPThermalScatteringData::GetX ( const G4DynamicParticle* aP, G4double aT , std::map < G4double , G4NeutronHPVector* >* amapTemp_EnergyCross )
{
   G4double result = 0;
   if ( amapTemp_EnergyCross->size() == 0 ) return result;

   std::map< G4double , G4NeutronHPVector* >::iterator it; 
   for ( it = amapTemp_EnergyCross->begin() ; it != amapTemp_EnergyCross->end() ; it++ )
   {
       if ( aT < it->first ) break;
   } 
   if ( it == amapTemp_EnergyCross->begin() ) it++;  // lower than first
      else if ( it == amapTemp_EnergyCross->end() ) it--;  // upper than last

   G4double eKinetic = aP->GetKineticEnergy();

   G4double TH = it->first;
   G4double XH = it->second->GetXsec ( eKinetic ); 

   //G4cout << "G4NeutronHPThermalScatteringData::GetX TH " << TH << " E " << eKinetic <<  " XH " << XH << G4endl;

   it--;
   G4double TL = it->first;
   G4double XL = it->second->GetXsec ( eKinetic ); 

   //G4cout << "G4NeutronHPThermalScatteringData::GetX TL " << TL << " E " << eKinetic <<  " XL " << XL << G4endl;

   if ( TH == TL )  
      throw G4HadronicException(__FILE__, __LINE__, "Thermal Scattering Data Error!");  

   G4double T = aT;
   G4double X = ( XH - XL ) / ( TH - TL ) * ( T - TL ) + XL;
   result = X;
  
   return result;
}


