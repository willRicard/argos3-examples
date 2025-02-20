/* Include the controller definition */
#include "crazyflie_sensing.h"
/* Function definitions for XML parsing */
#include <argos3/core/utility/configuration/argos_configuration.h>
/* 2D vector definition */
#include <argos3/core/utility/math/vector2.h>
/* Logging */
#include <argos3/core/utility/logging/argos_log.h>

/****************************************/
/****************************************/

CCrazyflieSensing::CCrazyflieSensing() :
   m_pcDistance(NULL),
   m_pcPropellers(NULL),
   m_pcRNG(NULL),
   m_pcRABA(NULL),
   m_pcRABS(NULL),
   m_pcPos(NULL),
   m_pcBattery(NULL),
   m_uiCurrentStep(0) {}

/****************************************/
/****************************************/

void CCrazyflieSensing::Init(TConfigurationNode& t_node) {
   try {
      /*
       * Initialize sensors/actuators
       */
      m_pcDistance   = GetSensor  <CCI_CrazyflieDistanceScannerSensor>("crazyflie_distance_scanner");
      m_pcPropellers = GetActuator  <CCI_QuadRotorPositionActuator>("quadrotor_position");
      /* Get pointers to devices */
      m_pcRABA   = GetActuator<CCI_RangeAndBearingActuator>("range_and_bearing");
      m_pcRABS   = GetSensor  <CCI_RangeAndBearingSensor  >("range_and_bearing");
      try {
         m_pcPos = GetSensor  <CCI_PositioningSensor>("positioning");
      }
      catch(CARGoSException& ex) {}
      try {
         m_pcBattery = GetSensor<CCI_BatterySensor>("battery");
      }
      catch(CARGoSException& ex) {}      
   }
   catch(CARGoSException& ex) {
      THROW_ARGOSEXCEPTION_NESTED("Error initializing the crazyflie sensing controller for robot \"" << GetId() << "\"", ex);
   }
   /*
    * Initialize other stuff
    */
   /* Create a random number generator. We use the 'argos' category so
      that creation, reset, seeding and cleanup are managed by ARGoS. */
   m_pcRNG = CRandom::CreateRNG("argos");

   m_uiCurrentStep = 0;
   Reset();
}

/****************************************/
/****************************************/

void CCrazyflieSensing::ControlStep() {
   // Dummy behavior: takeoff for 10 steps, 
   // then moves in a square shape for 200 steps then lands.

   int nInitSteps = 10;
   int nTotalSteps = 400;
   // Takeoff
   if ( m_uiCurrentStep < nInitSteps ) {
      TakeOff();
      m_cInitialPosition = m_pcPos->GetReading().Position;
   } 
   else if ((m_uiCurrentStep - nInitSteps) < nTotalSteps) {
      // Square pattern
      CVector3 trans(0.0f, 0.0f, 0.0f);
      if ( (m_uiCurrentStep - nInitSteps) < nTotalSteps/4 ) {
         trans.SetX(1.0f);
      }
      else if ( (m_uiCurrentStep - nInitSteps) < 2*nTotalSteps/4 ) {
         trans.SetY(1.0f);
      }
      else if ( (m_uiCurrentStep - nInitSteps) < 3*nTotalSteps/4 ) {
         trans.SetX(-1.0f);
      }
      else {
         trans.SetY(-1.0f);
      }
      CVector3 currentPosition = m_pcPos->GetReading().Position;
      CVector3 relativePositionCommand = (m_cInitialPosition + trans) - currentPosition; 
      
      m_pcPropellers->SetRelativePosition(relativePositionCommand);
   }
   else {
      Land();
   }   
   // Print current position.
   LOG << "Position (x,y,z) = (" << m_pcPos->GetReading().Position.GetX() << ","
       << m_pcPos->GetReading().Position.GetY() << ","
       << m_pcPos->GetReading().Position.GetZ() << ")" << std::endl; 

   // Print current battery level
   const CCI_BatterySensor::SReading& sBatRead = m_pcBattery->GetReading();
   LOG << "Battery level: " << sBatRead.AvailableCharge  << std::endl;

   // Look here for documentation on the distance sensor: /root/argos3/src/plugins/robots/crazyflie/control_interface/ci_crazyflie_distance_scanner_sensor.h
   // Read and print distance sensor measurements
   CCI_CrazyflieDistanceScannerSensor::TReadingsMap sDistRead = 
      m_pcDistance->GetReadingsMap();
   auto iterDistRead = sDistRead.begin();
   if (sDistRead.size() == 4) {
      LOG << "Front dist: " << (iterDistRead++)->second  << std::endl;
      LOG << "Left dist: "  << (iterDistRead++)->second  << std::endl;
      LOG << "Back dist: "  << (iterDistRead++)->second  << std::endl;
      LOG << "Right dist: " << (iterDistRead)->second  << std::endl;
   }

   // Increase step counter
   m_uiCurrentStep++;
}

/****************************************/
/****************************************/

bool CCrazyflieSensing::TakeOff() {
   CVector3 cPos = m_pcPos->GetReading().Position;
   if(Abs(cPos.GetZ() - 2.0f) < 0.01f) return false;
   cPos.SetZ(2.0f);
   m_pcPropellers->SetAbsolutePosition(cPos);
   return true;
}

/****************************************/
/****************************************/

bool CCrazyflieSensing::Land() {
   CVector3 cPos = m_pcPos->GetReading().Position;
   if(Abs(cPos.GetZ()) < 0.01f) return false;
   cPos.SetZ(0.0f);
   m_pcPropellers->SetAbsolutePosition(cPos);
   return true;
}

/****************************************/
/****************************************/

void CCrazyflieSensing::Reset() {
}

/****************************************/
/****************************************/

/*
 * This statement notifies ARGoS of the existence of the controller.
 * It binds the class passed as first argument to the string passed as
 * second argument.
 * The string is then usable in the XML configuration file to refer to
 * this controller.
 * When ARGoS reads that string in the XML file, it knows which controller
 * class to instantiate.
 * See also the XML configuration files for an example of how this is used.
 */
REGISTER_CONTROLLER(CCrazyflieSensing, "crazyflie_sensing_controller")
