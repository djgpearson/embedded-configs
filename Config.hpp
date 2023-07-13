#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <assert.h>
#include <tuple>
#include <stdint.h>
#include <string.h>
#include <algorithm>


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


typedef uint16_t ConfigFieldSizeStorageType;

extern void test();

struct ConfigField
{
  bool has_value;
  
  //If you see a linker message that these are missing, you're calling the wrong function.
  virtual void serialize(uint8_t* buf) const = 0;
  virtual size_t get_serialized_size() const = 0;
  virtual ConfigFieldSizeStorageType deserialize(const uint8_t* buf, size_t size) = 0;
};

template <class T, size_t T_ARR_LEN>
struct ArrayField : public ConfigField
{
  static constexpr size_t serialized_size = sizeof(ConfigFieldSizeStorageType) + (sizeof(T) * T_ARR_LEN);
  
  T value[T_ARR_LEN];
  
  
  void serialize(uint8_t* buf) const override
  {
    //Write out size!
    memcpy(buf, &serialized_size, sizeof(ConfigFieldSizeStorageType));
    buf += sizeof(ConfigFieldSizeStorageType);
    memcpy(buf, value, sizeof(value));
  }
  size_t get_serialized_size() const override
  {
    return serialized_size;
  }
  
  ConfigFieldSizeStorageType deserialize(const uint8_t* buf, size_t size) override
  {
    ConfigFieldSizeStorageType diskSize;
    memcpy(&diskSize, buf, sizeof(ConfigFieldSizeStorageType));
    buf += sizeof(ConfigFieldSizeStorageType);
    
    size_t copySize = diskSize - sizeof(ConfigFieldSizeStorageType);
    if(copySize > sizeof(value))
    {
      copySize = sizeof(value);
    }
    
    memcpy(value, buf, copySize);
    return diskSize;
  }
  
  
};


template <class T>
struct PODField : public ConfigField
{
  static constexpr size_t serialized_size = sizeof(ConfigFieldSizeStorageType) + sizeof(T);
  
  T value;
  
  void set_value(const T& v)
  {
    value = v;
  }
  
  void serialize(uint8_t* buf) const override
  {
    //Write out size!
    memcpy(buf, &serialized_size, sizeof(ConfigFieldSizeStorageType));
    buf += sizeof(ConfigFieldSizeStorageType);
    memcpy(buf, &value, sizeof(T));
  }
  size_t get_serialized_size() const override
  {
    return serialized_size;
  }
  
  ConfigFieldSizeStorageType deserialize(const uint8_t* buf, size_t size) override
  {
    ConfigFieldSizeStorageType diskSize;
    memcpy(&diskSize, buf, sizeof(ConfigFieldSizeStorageType));
    buf += sizeof(ConfigFieldSizeStorageType);
    
    size_t copySize = diskSize - sizeof(ConfigFieldSizeStorageType);
    if(copySize > sizeof(value))
    {
      copySize = sizeof(value);
    }
    
    memcpy(&value, buf, copySize);
    return diskSize;
  }
  
  
};

typedef PODField<int32_t> IntField;
typedef PODField<uint32_t> UintField;
typedef PODField<bool> BoolField;

template <size_t T_SIZE>
struct StringField : public ConfigField
{
  static constexpr size_t serialized_size = sizeof(ConfigFieldSizeStorageType) + T_SIZE + 1;
  
  //Add one for null terminator
  char value[T_SIZE + 1];
  
  void set_value(const char* str)
  {
    strncpy(value, str, T_SIZE);
    value[T_SIZE] = '\0';
  }
  
  void serialize(uint8_t* buf) const override
  {
    //Write out size!
    memcpy(buf, &serialized_size, sizeof(ConfigFieldSizeStorageType));
    buf += sizeof(ConfigFieldSizeStorageType);
    memcpy(buf, value, sizeof(value));
  }
  size_t get_serialized_size() const override
  {
    return serialized_size;
  }
  
  ConfigFieldSizeStorageType deserialize(const uint8_t* buf, size_t size) override
  {
    ConfigFieldSizeStorageType diskSize;
    memcpy(&diskSize, buf, sizeof(ConfigFieldSizeStorageType));
    buf += sizeof(ConfigFieldSizeStorageType);
    
    size_t copySize = diskSize - sizeof(ConfigFieldSizeStorageType);
    if(copySize > sizeof(value))
    {
      copySize = sizeof(value);
    }
    
    memcpy(value, buf, copySize);
    value[T_SIZE] = '\0';
    
    return diskSize;
  }
  
};


//The recursive for-each on the tuple members was from this: https://stackoverflow.com/a/16388876/3908226
template <typename EnumClass, class...Types>
class Config : public ConfigField
{
private:
  std::tuple<Types...> _fields;
  
  template <typename T>
    void serialize_field(const T& field, uint8_t** buffer) const
    {
      field.serialize(*buffer);
      *buffer = &((*buffer)[T::serialized_size]);
    }
  
  template <typename T>
    void deserialize_field(T& field, const uint8_t** buffer, ConfigFieldSizeStorageType& size)
    {
      //Ensure we have enough space left for our next size value
      if(size < sizeof(ConfigFieldSizeStorageType))
      {
	assert(false);
	size = 0;
	return;
      }
      
      ConfigFieldSizeStorageType consumedSize = field.deserialize(*buffer, size);
      if(consumedSize > size)
      {
	assert(false);
	//Abort, bad state!!
	size = 0;
	return;
      }
      else
      {
      	size -= consumedSize;
      	*buffer = *buffer + consumedSize;
      }
    }
  
  
  template <int,typename Arg,typename...Args>
    void serialize_each_field_helper(uint8_t** buf) const
    {
      serialize_each_field_helper<0,Args...>(buf);
      serialize_field<sizeof...(Args)>(buf);
    }
  
  template <int,typename Arg,typename...Args>
    void deserialize_each_field_helper(const uint8_t** buf, ConfigFieldSizeStorageType& size)
    {
      deserialize_each_field_helper<0,Args...>(buf, size);
      deserialize_field<sizeof...(Args)>(buf, size);
    }
  
  // Anchor for the recursion
  template <int>
    void serialize_each_field_helper(uint8_t** buf) const { }
  template <int>
    void deserialize_each_field_helper(const uint8_t** buf, ConfigFieldSizeStorageType& size) { }
  
public:
  
  static constexpr size_t serialized_size = sizeof(ConfigFieldSizeStorageType) + (Types::serialized_size + ...);
  
  static constexpr size_t root_config_size_size = sizeof(ConfigFieldSizeStorageType);
  
  template <EnumClass N>
    typename std::tuple_element<(size_t)N, std::tuple<Types...>>::type& get()
    {
      return std::get<(size_t)N>(_fields);
    }
  
  
  size_t read_root_config_size(const uint8_t* data)
  {
    ConfigFieldSizeStorageType size;
    memcpy(&size, data, sizeof(ConfigFieldSizeStorageType));
    return size;
  }
  
  void serialize(uint8_t* buf) const override
  {
    //First N bytes of every field should be the field size.
    memcpy(buf, &serialized_size, sizeof(ConfigFieldSizeStorageType));
    buf = buf + sizeof(ConfigFieldSizeStorageType);
    serialize_each_field_helper<0, Types...>(&buf); 
  }
  
  ConfigFieldSizeStorageType deserialize(const uint8_t* buf, size_t size) override
  {
    if(size < sizeof(ConfigFieldSizeStorageType))
    {
      has_value = false;
      return size;
    }
    
    //read in the size!
    ConfigFieldSizeStorageType diskSize;
    memcpy(&diskSize, buf, sizeof(ConfigFieldSizeStorageType));
    buf = buf + sizeof(ConfigFieldSizeStorageType);
    
    ConfigFieldSizeStorageType mutableDiskSize = diskSize - sizeof(ConfigFieldSizeStorageType);
    if(mutableDiskSize)
    {
    	deserialize_each_field_helper<0, Types...>(&buf, mutableDiskSize); 
	has_value = true;
    }
    else
    {
      has_value = false;
    }
    return diskSize;
  }
  
  template<size_t N>
    void serialize_field(uint8_t** buf) const
    {
      auto& field = std::get<N>(_fields);
      serialize_field(field, buf);
    }
  
  template<size_t N>
    void deserialize_field(const uint8_t** buf, ConfigFieldSizeStorageType& size)
    {
      auto& field = std::get<N>(_fields);
      
      if(size)
      {
	deserialize_field(field, buf, size);
	field.has_value = true;
      }
      else
      {
	field.has_value = false;
      }
    }
  
  size_t get_serialized_size() const override
  {
    return serialized_size;
  }
  
  
  
  
  
};


#endif