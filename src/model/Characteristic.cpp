#include "model/Characteristic.h"

#include "engine/model/RoleSet.h"
#include "engine/model/Task.h"
#include "engine/AlicaEngine.h"

#include <exception>
#include <iostream>
#include <sstream>

namespace alica
{

float Characteristic::calculateSimilarityTo(Characteristic* characteristic) 
{
        float similarity = 1.0f -
                        (levenshteinDistance(this->getName(), characteristic->getName())
                                / (this->getName().length() > characteristic->getName().length() ? this->getName().length() : characteristic->getName().length()));

        if (isNumeric(_value) ^ isNumeric(characteristic->getValue())) {
            similarity = 0;
        }
        else {
            similarity += 1.0f -
                    (levenshteinDistance(_value, characteristic->getValue())
                            / (_value.length() > characteristic->getValue().length() ? _value.length() : characteristic->getValue().length()));
            similarity/=2;
        }

        //std::cout << "C: Are numerics? " << _value <<" "<< true <<" " << isNumeric(_value)<< " " << (isNumeric(_value)? "true": "false")  << "   " << characteristic->getValue() << " " << (isNumeric(characteristic->getValue())?"true": "false") << std::endl;
        //std::cout << "C: " << this->getName() << ":" << _value << "     " << characteristic->getName() << ":" + characteristic->getValue() <<   "     " << similarity  << std::endl;

        return similarity;
}

float Characteristic::levenshteinDistance (const std::string& s1, const std::string& s2)
{
    const std::size_t len1 = s1.size(), len2 = s2.size();
	std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

	d[0][0] = 0;
	for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
	for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

	for(unsigned int i = 1; i <= len1; ++i)
		for(unsigned int j = 1; j <= len2; ++j)
                      // note that std::min({arg1, arg2, arg3}) works only in C++11,
                      // for C++98 use std::min(std::min(arg1, arg2), arg3)
                      d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) });

    //std::cout << "C: implementation is missing" << std::endl;
    return d[len1][len2];
}

bool Characteristic::isNumeric(const std::string& value)
{
    //std::cout << "C: implementation is missing" << std::endl;
    // char* p;
    // strtol(value.c_str(), &p, 10);
    // return *p == 0;
    return std::all_of(value.begin(), value.end(), ::isdigit);

}


std::string Characteristic::toString(std::string indent) const
{
    std::stringstream ss;
    ss << indent << "#Characteristic: " << getName() << " " << getId() << " " << _value << " " << _weight << std::endl;
    return ss.str();
}

} // namespace alica
