#include "stdafx.h"
#include "ServerGroup.h"
#include "ServerDisplayNames.h"

// cppcheck-suppress uninitMemberVar
CServerGroup::CServerGroup()
{
    m_iSequence = 0;
    m_iServerIndex = 0;
    m_iNumServer = 0;
    // A group only gets a button position once UpdateDisplay() places it, and
    // it skips groups that do not fit; -1 says "no button" until then.
    m_iBtnPos = -1;

    for (int i = 0; i < MAX_SERVER_PER_GROUP; i++)
    {
        m_abyNonPvpServer[i] = 0;
    }
}

CServerGroup::~CServerGroup()
{
    Release();
}

void CServerGroup::Release()
{
    auto iterServer = m_listServerInfo.begin();
    for (; iterServer != m_listServerInfo.end(); iterServer++)
    {
        delete (*iterServer);
    }

    m_listServerInfo.clear();
}

void CServerGroup::InsertServerInfo(CServerInfo* pServerInfo)
{
    // Listed by display rank (not by arrival order of the server list), then numbered by position: the select menu
    // finds the clicked server by that position (GetServerInfo).
    const int rank = Network::ServerNames::ServerDisplayRank(pServerInfo->m_iIndex);
    auto position = m_listServerInfo.begin();
    while (position != m_listServerInfo.end() && Network::ServerNames::ServerDisplayRank((*position)->m_iIndex) <= rank)
    {
        ++position;
    }

    m_listServerInfo.insert(position, pServerInfo);

    int sequence = 0;
    for (CServerInfo* server : m_listServerInfo)
    {
        server->m_iSequence = sequence++;
    }

    m_iterServerList = m_listServerInfo.begin();

    m_iNumServer = m_listServerInfo.size();
}

void CServerGroup::SetFirst()
{
    m_iterServerList = m_listServerInfo.begin();
}

bool CServerGroup::GetNext(OUT CServerInfo*& pServerInfo)
{
    if (m_iterServerList == m_listServerInfo.end())
    {
        pServerInfo = NULL;

        return false;
    }

    pServerInfo = (*m_iterServerList);

    m_iterServerList++;

    return true;
}

int CServerGroup::GetServerSize()
{
    return m_listServerInfo.size();
}

CServerInfo* CServerGroup::GetServerInfo(int iSequence)
{
    auto iterServer = m_listServerInfo.begin();

    while (iterServer != m_listServerInfo.end())
    {
        if ((*iterServer)->m_iSequence == iSequence)
            return (*iterServer);

        iterServer++;
    }

    return NULL;
}