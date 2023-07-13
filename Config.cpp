#include "Config.hpp"



enum class VehicleRecFields
{
  EcuSn,
  Vin,
  LastFlashedTune,
  TuneCompat,
  ChecksumCompat,
  RecoveryMode,
  FlashCount
};

typedef Config<VehicleRecFields, 
	StringField<50>, // ECU SN
	StringField<20>, // VIN
	StringField<255>, // LastFlashedTune
	StringField<30>, // tune compat
	StringField<30>, //checksumCompat
	BoolField
	> ConnectedVehicleConfig;

typedef Config<VehicleRecFields, 
	StringField<70>, // ECU SN
	StringField<20>, // VIN
	StringField<255>, // LastFlashedTune
	StringField<30>, // tune compat
	StringField<30>, //checksumCompat
	BoolField, // recovery mode
	UintField // flash counter
	> UpgradedConnectedVehicleConfig;



//typedef  Config<TestFields, StringField<10>, StringField<20>, BoolField, IntField, ArrayField<bool, 10>> TestConfigType;

void test()
{
  
  
  ConnectedVehicleConfig config;
  
  //ECU SN
  config.get<VehicleRecFields::EcuSn>()
    .set_value("AA BB CC DD EE");
  
  //VIN
  config.get<VehicleRecFields::Vin>()
    .set_value("1DJCAFECAFECAFECAFECAFECAFE");
  
  //RECOVERY MODE FLAG
  config.get<VehicleRecFields::RecoveryMode>()
    .set_value(false);
  
  //Note that config size can be determined AT COMPILE TIME because templates
  uint8_t writeBuffer[config.serialized_size];
  memset(writeBuffer, 0, sizeof(writeBuffer));
  
  config.serialize(writeBuffer);
  
  
  //Read the config back into an "upgraded" version of the config - deliberately add fields
  //And change lengths on the same config type to test upgrading/changing firmware later
  UpgradedConnectedVehicleConfig readConfig;
  
  //dynamically read size since we're acting like new firmware and OLD firmware was different
  size_t diskSize = readConfig.read_root_config_size(writeBuffer);
  
  //Deserialize
  readConfig.deserialize(writeBuffer, diskSize);
  
  if(readConfig.has_value)
  {
  	assert(readConfig.get<VehicleRecFields::EcuSn>().has_value);
  	assert(strcmp(readConfig.get<VehicleRecFields::EcuSn>().value, "AA BB CC DD EE"));
  }
  
  
  
}

