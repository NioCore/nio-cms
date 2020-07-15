#pragma once

#include <engine/IRoleAssignment.h>

namespace essentials {
    class IdentifierConstPtr;
}

namespace alica
{
class ActiveAgentView;
class Agent;
class AlicaEngine;
class RobotPrsoperties;
class Role;
class RoleUtility;

class DynamicRoleAssignment : public IRoleAssignment
{
public:
    DynamicRoleAssignment(AlicaEngine* ae);
    ~DynamicRoleAssignment() = default;

    void init() override;
    void tick() override;
    void update() override;

    /**
     * Calculates the actual role assignment and is triggered if an
     * update is deemed necessary.
     */
    void calculateRoles();

private:
    double calculateMatching(const RobotProperties&, const Role&);
    void mapRoleToAgent();
    void printAvailableAgents();

    AlicaEngine* _ae;
    bool _updateRoles;

    //ActiveAgentView* _activeAgents;
    std::vector<RoleUtility*> _sortedAgentRoleUtility;
    const Agent* _localAgent;
};

} /* namespace alica */
