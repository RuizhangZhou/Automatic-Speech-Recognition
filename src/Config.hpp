/*****************************************************************************/
/*                                                                           */
/*       COPYRIGHT (C) 2015 Lehrstuhl fuer Informatik VI, RWTH Aachen        */
/*                                                                           */
/*****************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <rapidjson/stream.h>

class Configuration {
private:
  struct Internal;
public:
  Configuration();
  Configuration(std::unique_ptr<Internal>&& internal);
  Configuration(Configuration&& other);
  Configuration(std::string const& path);
  Configuration(rapidjson::StringStream& json);
  ~Configuration();

  bool has_value(std::string const& name) const;

  template<typename T>
  bool is_type(std::string const& name) const;

  template<typename T>
  T get_value(std::string const& name) const;

  bool is_array(std::string const& name) const;
  template<typename T> std::vector<T> get_array(std::string const& name) const;
  Configuration sub_config(std::string const& name) const;

  void write_to_stream(std::ostream& out) const;
private:
  std::unique_ptr<Internal> internal_;
};

template<typename T>
class Parameter {
  public:
    Parameter(std::string const& name, T const& default_value);
    ~Parameter();

    T operator()(Configuration const& config) const;
  private:
    std::string name_;
    T default_value_;
};

typedef Parameter<bool>        ParameterBool;
typedef Parameter<int32_t>     ParameterInt;
typedef Parameter<uint32_t>    ParameterUInt;
typedef Parameter<int64_t>     ParameterInt64;
typedef Parameter<uint64_t>    ParameterUInt64;
typedef Parameter<float>       ParameterFloat;
typedef Parameter<double>      ParameterDouble;
typedef Parameter<std::string> ParameterString;

#endif /* __CONFIG_H__ */
