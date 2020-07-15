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

DynamicRoleAssignment::DynamicRoleAssignment(AlicaEngine* ae)
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
    this->_localAgent = _ae->getTeamManager()->getLocalAgent();
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
    std::cout << "DRA: calculate matching " << std::endl;
    double matching = 0;
        std::cout << "DRA: Characteristic simiarity " << agentProperties.getDefaultRole() << "  " << role.getName() << std::endl;
        //std::cout << "DRA: Characteristic size " << role.getCharacteristics().size() << std::endl;

        for (std::pair<std::string, Characteristic*> roleCharacteristic : role.getCharacteristics()) {
            //std::cout << "DRA: role characteristic name " << roleCharacteristic.first << std::endl;
            double similarity = 0;
            std::string name;
            //std::cout << "DRA: agent CharacteristicS size " << agentProperties.getCharacteristics().size() << std::endl;

            for (std::pair<std::string, Characteristic*> agentCharacteristic : agentProperties.getCharacteristics()) {
                //std::cout << "DRA: similarity " << agentCharacteristic.second->getValue()<< " " << roleCharacteristic.second->getValue() << std::endl;

                if (agentCharacteristic.second->getValue().empty() || roleCharacteristic.second->getValue().empty())
                    continue;
                double _similarity = agentCharacteristic.second->calculateSimilarityTo(roleCharacteristic.second) * roleCharacteristic.second->getWeight();
                //std::cout << "DRA: similarity:" << _similarity << std::endl;

                if (similarity < _similarity) {
                    similarity = _similarity;
                    name = agentCharacteristic.second->getName();
                }
            }
            std::cout <<  "DRA:        characteristic:" << roleCharacteristic.second->getName() << "   similarity " << name << " " << similarity << std::endl;
            matching += similarity;
        }

        if (role.getCharacteristics().size() > 0) 
            matching /= role.getCharacteristics().size();
        
        if (matching == 0)
            matching /= NAN;
        
        return matching;
}

/**
 * Recalculates the complete mapping from robot to role.
 */
void DynamicRoleAssignment::calculateRoles()
{
    std::string name = _localAgent->getName();
    essentials::IdentifierConstPtr id = _ae->getTeamManager()->getLocalAgentID();
    essentials::SystemConfig* sc = essentials::SystemConfig::getInstance();
    //  essentials::IdentifierConstPtr id2 =  _localAgent->getProperties().extractID(name, sc);
    // std::cout << "\033[0;34m" << "DRA:: ---------------------------" << "\033[0m" << std::endl;
    // std::cout <<"\033[0;34m" << "DRA:: " << name << "  "<< id << "  = " << id2 << "\033[0m" << std::endl;
    // std::cout << "\033[0;34m" << "DRA:: ---------------------------" << "\033[0m" << std::endl;

    const RoleSet* roleSet = _ae->getRoleSet();

    if (roleSet == nullptr){
        // std::cout << "DRA(" << name << "  " << _localAgent->getProperties().extractID(name, sc) << "): The current Roleset is null!" << std::endl;
        std::cout << "DRA(" << id.get()->hash() << "): The current Roleset is null!" << std::endl;
    }

    const PlanRepository::Accessor<Role>& roles = _ae->getPlanRepository()->getRoles();

    //_activeAgents = _ae->getTeamManager()->getActiveAgents();
    _sortedAgentRoleUtility.clear();

        this->printAvailableAgents();

        for (const Role* role : roles) {

            for (const Agent* agent : _ae->getTeamManager()->getActiveAgents()) {

                double matching = this->calculateMatching(agent->getProperties(), *role);

                std::cout << "DRA("<< id.get()->hash() << "): characteristic size:" << agent->getProperties().getCharacteristics().size() << "    matching " << matching << "\n" << std::endl;
                // std::cout << "DRA(" << name << "  "<< _localAgent->getProperties().extractID(name, sc) << "): characteristic size:" << agent->getProperties().getCharacteristics().size() << "    matching " << matching << "\n" << std::endl;

                if (matching != 0) {
                    RoleUtility* utility = new RoleUtility(matching, agent, role);
                    _sortedAgentRoleUtility.push_back(utility);
                }
            }
        }
        std::sort(_sortedAgentRoleUtility.begin(), _sortedAgentRoleUtility.end(),  [] ( RoleUtility * a, RoleUtility *  b) { return a->getUtilityValue() > b->getUtilityValue(); });
        //_sortedAgentRoleUtility.sort(Comparator.comparingDouble(RoleUtility::getUtilityValue).reversed());

        if (_sortedAgentRoleUtility.size() == 0) {
            std::cout << "DRA(" << id.get()->hash() << "): Could not establish a mapping between agents and roles. Please check capability definitions!"<< std::endl;
            // std::cout << "DRA(" << name << "  "<< _localAgent->getProperties().extractID(name, sc) << "): Could not establish a mapping between agents and roles. Please check capability definitions!"<< std::endl;
        }
        this->robotRoleMapping.clear();
        this->mapRoleToAgent();
    }

 void DynamicRoleAssignment::mapRoleToAgent() {
        std::string name = _localAgent->getName();
        essentials::IdentifierConstPtr localId = _ae->getTeamManager()->getLocalAgentID();
        essentials::SystemConfig* sc = essentials::SystemConfig::getInstance();
//        this.sortedAgentRoleUtility.sort(Comparator.comparing((RoleUtility u) -> u.getRole().getID())
//                                                   .thenComparing(u -> u.getUtilityValue())
//                                                   .thenComparing(u -> u.getAgentProperties().extractID()));

        bool roleIsAssigned = false;

        for (RoleUtility * agentRoleUtility : _sortedAgentRoleUtility) {
            // essentials::IdentifierConstPtr __id = agentRoleUtility->getAgent()-> getProperties().extractID(name, sc);
            essentials::IdentifierConstPtr __id = agentRoleUtility->getAgent()->getId();

            //std::cout << "DRA(" << localId << "): agentID: "    << agentRoleUtility->getAgent()->getProperties().extractID(name, sc) << std::endl;
            std::cout << "DRA(" << localId.get()->hash() << "):   agent: "  << __id.get()->hash() << "    "  << agentRoleUtility->getAgent()->getProperties().getName() << "  " << agentRoleUtility->getRole()->getName()<< std::endl;
            // std::cout << "DRA(" << name << "  "<< _localAgent->getProperties().extractID(name, sc) << "): agentID:" 
            //         << agentRoleUtility->getAgent()->getProperties().extractID(name, sc) << std::endl;
            // std::cout << "DRA(" << name << "  "<< _localAgent->getProperties().extractID(name, sc) << "):   agent:  " 
            //         << agentRoleUtility->getAgent()->getProperties().getName() << "  " << agentRoleUtility->getRole()->getName()<< std::endl;

            if (this->robotRoleMapping.size() != 0
                    && (this->robotRoleMapping.at(__id) != nullptr
                    || agentRoleUtility->getUtilityValue() == 0)) {
                 std::cout << "DRA(" << localId.get()->hash() << "):     continue (" << this->robotRoleMapping.size() 
                        << "!= 0)" << " && " << this->robotRoleMapping.at(__id) << "!= null" 
                        << " || " << agentRoleUtility->getUtilityValue() << "== 0"<< std::endl;
                //  std::cout << "DRA(" << name << "  "<< _localAgent->getProperties().extractID(name, sc) << "):     continue (" << this->robotRoleMapping.size() 
                //         << "!= 0)" << " && " << this->robotRoleMapping.at(agentRoleUtility->getAgent()->getProperties().extractID(name, sc)) << "!= null" 
                //         << " || " << agentRoleUtility->getUtilityValue() << "== 0"<< std::endl;
                continue;
            }
            //  std::cout << "DRA(" << name << "  "<< _localAgent->getProperties().extractID(name, sc) << "):     put teamObserver mapping (agentID:" 
            //         << agentRoleUtility->getAgent()->getProperties().extractID(name, sc) << " role:" + agentRoleUtility->getRole()->getName() << ")"<< std::endl;
             std::cout << "DRA(" << localId.get()->hash() << "):     put teamObserver mapping (agentID:" 
                    << __id.get()->hash() << " role:" + agentRoleUtility->getRole()->getName() << ")"<< std::endl;

            
            const Role* role = agentRoleUtility->getRole();
            this->robotRoleMapping.insert( { __id, role });

            if (localId == __id) {
                 std::cout << "DRA(" << __id.get()->hash() << "):       set teamObserver own role (agentID:" 
                    << __id.get()->hash() << " role:" << agentRoleUtility->getRole()->getName() << ")"<< std::endl;
                //  std::cout << "DRA(" << name << "  "<< _localAgent->getProperties().extractID(name, sc) << "):       set teamObserver own role (agentID:" 
                //     << agentRoleUtility->getAgent()->getProperties().extractID(name, sc) << " role:" << agentRoleUtility->getRole()->getName() << ")"<< std::endl;
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
                break;
            }
        }
        if (!roleIsAssigned) {
            //  std::cout << "DRA: Could not set a role for agent " << _localAgent->getProperties().getName() << " with default role " 
            //  << _localAgent->getProperties().getDefaultRole() << "!"<< std::endl;
             std::cout << "DRA: Could not set a role for agent " << localId.get()->hash() << " with default role " 
             << _localAgent->getProperties().getDefaultRole() << "!"<< std::endl;
        }
    }

    // -- debug outputs ------
    void DynamicRoleAssignment::printAvailableAgents() {
        std::string name = _localAgent->getName();
        essentials::IdentifierConstPtr id =  _ae->getTeamManager()->getLocalAgentID();
        essentials::SystemConfig* sc = essentials::SystemConfig::getInstance();

        std::cout << "DRA(" << id.get()->hash() << "): Available agents: " << _ae->getTeamManager()->getActiveAgents().size();
        std::cout <<"   agent Ids: ";
        std::cout <<"\nDRA(" << id.get()->hash() << "): " << _ae->getTeamManager()->getActiveAgents().size() << std::endl;
        std::cout <<"DRA(";
        std::cout << id.get()->hash() << "): "; 
        // std::cout << "DRA("  << name << "  "<< name << "  "<< _localAgent->getProperties().extractID(name, sc) << "): Available agents: " << _ae->getTeamManager()->getActiveAgents().size();
        // std::cout <<"   agent Ids: ";
        // std::cout <<"\nDRA(" << _localAgent->getProperties().extractID(name, sc) << "): " << _ae->getTeamManager()->getActiveAgents().size() << std::endl;
        // std::cout <<"DRA(";
        // std::cout << name << "  "<<_localAgent->getProperties().extractID(name, sc) <<"): "; 
        
        for (const Agent* agent : _ae->getTeamManager()->getActiveAgents()) {
            std::cout <<  agent->getId() << " , ";
        }
        std::cout << std::endl;

        for (const Agent* agent : _ae->getTeamManager()->getActiveAgents()) {
            std::cout << id.get()->hash() << ":" << agent->getProperties().getName() << " , ";
            // std::cout << agent->getProperties().extractID(name, sc) << ":" << agent->getProperties().getName() << " , ";
        }
        std::cout << std::endl;
    }

} /* namespace alica */
