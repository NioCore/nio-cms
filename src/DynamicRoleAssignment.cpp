//#define STATIC_RA_DEBUG

#include "DynamicRoleAssignment.h"
#include "RoleUtility.h"
#include "model/Characteristic.h"

#include <engine/AlicaEngine.h>
#include <engine/IAlicaCommunication.h>
#include <engine/PlanRepository.h>
#include <engine/model/Role.h>
#include <engine/collections/RobotProperties.h>
#include <engine/containers/RoleSwitch.h>
#include <engine/teammanager/TeamManager.h>

#include <essentials/IdentifierConstPtr.h>

#include <alica_common_config/debug_output.h>

#include <cmath>




namespace alica
{

DynamicRoleAssignment::DynamicRoleAssignment(AlicaEngine * ae)
        : IRoleAssignment()
        ,_ae(ae)
        ,_updateRoles(false)
        ,_localAgent(nullptr)
{
}

/**
 * Initially calculates the robot-role mapping once.
 */
void DynamicRoleAssignment::init()
{
    std::cout <<  "\033[0;35m" << "DRA: init " << "\033[0m"<< std::endl;
    this->_localAgent = _ae->getTeamManager()->getLocalAgent();
     if (!_localAgent)
        std::cerr <<  "\033[0;35m" << "DRA: _localAgent does not exist" << "\033[0m"<< std::endl;
    this->calculateRoles();
}

/**
 * Triggers the recalculation of the robot-role mapping, if the updateRoles flag is set to true.
 */
void DynamicRoleAssignment::tick()
{
    if (_updateRoles) {
        _updateRoles = false;
        this->calculateRoles();
    }
}

/**
 * Sets the updateRoles flag to true, in order to recalculate the robot-role mapping on the next tick.
 */
void DynamicRoleAssignment::update()
{
    _updateRoles = true;
}

double DynamicRoleAssignment::calculateMatching(const RobotProperties& agentProperties, const Role& role)
{
    std::string _name = agentProperties.getName();
    essentials::SystemConfig* sc = essentials::SystemConfig::getInstance();
    std::cout <<  "\033[0;35m" << "DRA("<< _name << ")"<< "\033[0m"<< std::endl;
    auto _id = agentProperties.extractID(_name, sc);
    std::cout <<  "\033[0;35m" << "DRA("<< _name << ")"<< "\033[0m"<< std::endl;
    std::cout <<  "\033[0;35m" << "DRA("<< _id->hash() << "): calculate matching "  << "\033[0m"<< std::endl;
    double matching = 0;
    std::cout <<  "\033[0;35m" << "DRA("<< _id->hash() << "): Role similarity " << agentProperties.getDefaultRole() << "  " << role.getName()  << "\033[0m"<< std::endl;
    std::cout <<  "\033[0;35m" << "DRA("<< _id->hash() << "): Characteristic size " << role.getCharacteristics().size()  << "\033[0m"<< std::endl;

    for (std::pair<std::string, Characteristic*> roleCharacteristic : role.getCharacteristics()) {
            std::cout << "\033[0;35m" <<"DRA: role characteristic name " << roleCharacteristic.first << "\033[0m"<< std::endl;
            double similarity = 0;
            std::string name;
            std::cout << "\033[0;35m" <<"DRA: agent CharacteristicS size " << agentProperties.getCharacteristics().size() << "\033[0m"<< std::endl;

            for (std::pair<std::string, Characteristic*> agentCharacteristic : agentProperties.getCharacteristics()) {
                std::cout << "\033[0;35m" <<"DRA: similarity " << agentCharacteristic.second->getValue()<< " " << roleCharacteristic.second->getValue() << "\033[0m"<< std::endl;

                if (agentCharacteristic.second->getValue().empty() || roleCharacteristic.second->getValue().empty())
                    continue;
                double _similarity = agentCharacteristic.second->calculateSimilarityTo(roleCharacteristic.second) * roleCharacteristic.second->getWeight();
                std::cout << "\033[0;35m" <<"DRA: similarity:" << _similarity << "\033[0m"<< std::endl;

                if (similarity < _similarity) {
                    similarity = _similarity;
                    name = agentCharacteristic.second->getName();
                }
            }
            std::cout <<  "\033[0;35m" << "DRA("<< _id->hash() << "):        characteristic:" << roleCharacteristic.second->getName() << "   similarity " << name << " " << similarity  << "\033[0m"<< std::endl;
            matching += similarity;
    }
        
    if (role.getCharacteristics().size() > 0) 
            matching /= role.getCharacteristics().size();
    else {
        if (agentProperties.getDefaultRole() == role.getName())
            matching = 1;
    }    
    // if (matching == 0)
    //         matching /= NAN;
        
    std::cout << "\033[0;35m" <<"DRA: similarity:" << matching << "\033[0m"<< std::endl;
    return matching;
}

/**
 * Recalculates the complete mapping from robot to role.
 */
void DynamicRoleAssignment::calculateRoles()
{
    std::cout <<  "\033[0;35m" << "DRA: calculateRoles "  << "\033[0m"<< std::endl;
    if (!_localAgent)
        std::cerr <<  "\033[0;35m" << "DRA: _localAgent does not exist" << "\033[0m"<< std::endl;
    std::string name = _localAgent->getName();
    essentials::IdentifierConstPtr id = _ae->getTeamManager()->getLocalAgentID();
    essentials::SystemConfig* sc = essentials::SystemConfig::getInstance();
    //  essentials::IdentifierConstPtr id2 =  _localAgent->getProperties().extractID(name, sc);
    // std::cout << "\033[0;34m" << "DRA:: ---------------------------" << "\033[0m" << std::endl;
    // std::cout <<"\033[0;34m" << "DRA:: " << name << "  "<< id << "  = " << id2 << "\033[0m" << std::endl;
    // std::cout << "\033[0;34m" << "DRA:: ---------------------------" << "\033[0m" << std::endl;

    const RoleSet* roleSet = _ae->getRoleSet();

    if (roleSet == nullptr){
        std::cout << "DRA(" << id.get()->hash() << "): The current Roleset is null!" << std::endl;
    }

    const PlanRepository::Accessor<Role>& roles = _ae->getPlanRepository()->getRoles();

    //_activeAgents = _ae->getTeamManager()->getActiveAgents();
    _sortedAgentRoleUtility.clear();

    this->printAvailableAgents();

    for (const Role* role : roles) {

        for (const Agent* agent : _ae->getTeamManager()->getActiveAgents()) {

            double matching = this->calculateMatching(agent->getProperties(), *role);

            std::cout <<  "\033[0;35m" << "DRA("<< id.get()->hash() << "): characteristic size:" << agent->getProperties().getCharacteristics().size() << "    matching " << matching << "\n" << "\033[0m" << std::endl;

            if (matching != 0) {
                RoleUtility* utility = new RoleUtility(matching, agent, role);
                _sortedAgentRoleUtility.push_back(utility);
            }
        }
    }
    std::sort(_sortedAgentRoleUtility.begin(), _sortedAgentRoleUtility.end(),  [] ( RoleUtility * a, RoleUtility *  b) { return a->getUtilityValue() > b->getUtilityValue(); });
        //_sortedAgentRoleUtility.sort(Comparator.comparingDouble(RoleUtility::getUtilityValue).reversed());

    if (_sortedAgentRoleUtility.size() == 0) {
        std::cout <<  "\033[1m\033[31m" << "DRA(" << id.get()->hash() << "): Could not establish a mapping between agents and roles. Please check capability definitions!" << "\033[0m" << std::endl;
    }
    this->robotRoleMapping.clear();
    this->mapRoleToAgent();
}

 void DynamicRoleAssignment::mapRoleToAgent() {
        std::string name = _localAgent->getName();
        essentials::IdentifierConstPtr localId = _ae->getTeamManager()->getLocalAgentID();
        essentials::SystemConfig* sc = essentials::SystemConfig::getInstance();

        bool roleIsAssigned = false;
        std::cout<<  "\033[1m\033[35m" << "DRA(" << localId.get()->hash() << "): agent to role map size:" << _sortedAgentRoleUtility.size() << "\033[0m" << std::endl;

        for (RoleUtility * agentRoleUtility : _sortedAgentRoleUtility) {
            // essentials::IdentifierConstPtr __id = agentRoleUtility->getAgent()-> getProperties().extractID(name, sc);
            essentials::IdentifierConstPtr __id = agentRoleUtility->getAgent()->getId();

            std::cout<<  "\033[1m\033[35m" << "DRA(" << localId.get()->hash() << "):   map role to agent" << "\033[0m" << std::endl;
            std::cout<<  "\033[1m\033[35m" << "DRA(" << localId.get()->hash() << "):   agent: "  << __id.get()->hash() << "    "  << agentRoleUtility->getAgent()->getProperties().getName() << "  " << agentRoleUtility->getRole()->getName() << "\033[0m" << std::endl;

            if (this->robotRoleMapping.size() != 0 
                    && this->robotRoleMapping.find(__id) != this->robotRoleMapping.end()
                    && (this->robotRoleMapping.at(__id) != nullptr
                    || agentRoleUtility->getUtilityValue() == 0)) {
                 std::cout <<  "\033[1m\033[35m" << "DRA(" << localId.get()->hash() << "):     continue:  robotrolemapping:" << this->robotRoleMapping.size() 
                        << "!= 0" << " && " << this->robotRoleMapping.at(__id) << "!= null" 
                        << " || " << agentRoleUtility->getUtilityValue() << "== 0" << "\033[0m" << std::endl;
                continue;
            }
             std::cout <<  "\033[1m\033[35m"  << "DRA(" << localId.get()->hash() << "):     put teamObserver mapping (agentID:" 
                    << __id.get()->hash() << " role:" + agentRoleUtility->getRole()->getName() << ")" << "\033[0m" << std::endl;

            
            const Role* role = agentRoleUtility->getRole();
            std::cout <<  "\033[1m\033[35m"  << "DRA(" << localId.get()->hash() << "):     role name:" << role->getName() << "\033[0m" << std::endl;
            this->robotRoleMapping.insert( { __id, role });

            if (localId == __id) {
                 std::cout <<  "\033[1m\033[35m"  << "DRA(" << __id.get()->hash() << "):       set teamObserver own role (agentID:" 
                    << __id.get()->hash() << " role:" << agentRoleUtility->getRole()->getName() << ")" << "\033[0m" << std::endl;
                this->ownRole = agentRoleUtility->getRole();

                if (this->communication != nullptr) {
                    RoleSwitch roleSwitch;
                    roleSwitch.roleID = this->ownRole->getId();
                    this->communication->sendRoleSwitch(roleSwitch);
                }
//            this.teamObserver.getAgentById(agentRoleUtility.getAgent().getProperties().extractID()).setCurrentRole(agentRoleUtility.getRole());
//            Agent agent = this.ae.getTeamManager().getAgentByID(agentRoleUtility.getAgent().getProperties().extractID(name, sc));
//            agent.getProperties().setCurrentRole(agentRoleUtility.getRole());
                roleIsAssigned = true;
                // break;
            }
        }
        if (!roleIsAssigned) {
             std::cout <<  "\033[1m\033[31m" << "DRA: Could not set a role for agent " << localId.get()->hash() << " with default role " 
             << _localAgent->getProperties().getDefaultRole() << "!" << "\033[0m" << std::endl;
        }
        std::cout<<  "\033[1m\033[35m" << "DRA(" << localId.get()->hash() << "): map role to agent FINISHED" << "\033[0m" << std::endl;
    }

    // -- debug outputs ------
    void DynamicRoleAssignment::printAvailableAgents() {
        std::string name = _localAgent->getName();
        essentials::IdentifierConstPtr id =  _ae->getTeamManager()->getLocalAgentID();
        essentials::SystemConfig* sc = essentials::SystemConfig::getInstance();

        std::cout <<  "\033[1m\033[35m" << "DRA(" << id.get()->hash() << "): Available agents: " << _ae->getTeamManager()->getActiveAgents().size() << "\033[0m" << std::endl;
        std::cout <<  "\033[1m\033[35m" << "DRA(" << id.get()->hash() << "): "<<"   agent Ids  : ";
        
        for (const Agent* agent : _ae->getTeamManager()->getActiveAgents()) {
            std::cout << agent->getId().get()->hash() << ", ";
        }
        std::cout << std::endl;
        std::cout <<  "\033[1m\033[35m" << "DRA(" << id.get()->hash() << "): "<<"   agent names: ";
        
        for (const Agent* agent : _ae->getTeamManager()->getActiveAgents()) {
            std::cout << agent->getProperties().getName() << ", ";
            // std::cout << agent->getProperties().extractID(name, sc) << ":" << agent->getProperties().getName() << " , ";
        }
        std::cout  << "\033[0m" << std::endl;
    }

} /* namespace alica */
