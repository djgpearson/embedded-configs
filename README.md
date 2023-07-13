# embedded-configs

//This teeny config library has the following goals:
// 1: Writing and reading configs should be doable with MINIMAL developer effort - no serialization work
// 2: Avoid any dynamic memory allocation in order to use the config library
// 3: Configs should be UPGRADEABLE - meaning field lengths may be increased or decreased in later versions of software without adverse effect
// 4: Configs may have fields added in later versions of software without adverse effect. 
// 5: Config serialization size should be determined at COMPILE TIME

//Constraints for backwards compatibility:
// All configs must be FIXED LENGTH to meet rule 5
// Configs MUST NOT be added anywhere except the END of the config set
// The indexing enumeration must not be re-ordered at any time. 
// All fields must be POD types
