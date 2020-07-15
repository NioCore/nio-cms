#pragma once

#include "engine/model/AlicaElement.h"

#include <unordered_map>
#include <string>


namespace alica
{

class Characteristic : public AlicaElement
{
public:
    Characteristic():_value(""),_weight(0) {};
    virtual ~Characteristic() {};

    float calculateSimilarityTo(Characteristic* characteristic);
    
    const std::string& getValue() const { return _value;}
    void setValue(std::string value) { _value = value;}

    double getWeight() { return _weight;}
    void setWeight(double weight) { _weight = weight;}
    
    std::string toString(std::string indent = "") const override;

private:
    float levenshteinDistance (const std::string& lhs, const std::string& rhs);
    bool isNumeric(const std::string& value);

    std::string _value;
    double _weight;
};

} // namespace alica
