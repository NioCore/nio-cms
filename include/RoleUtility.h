#pragma once
namespace alica
{
class Agent;
class Role;

class RoleUtility
{
public:
    RoleUtility(double utilityValue, const Agent* agent, const Role* role)
            :_utilityValue(utilityValue),_agent(agent),_role(role){}
    virtual ~RoleUtility() {}

    const Role* getRole() { return _role;}
    const Agent* getAgent() {return _agent;}
    const double getUtilityValue() {return _utilityValue;}

private:
    const Agent* _agent;
    const Role* _role;
    const double _utilityValue;
};

} // namespace alica
