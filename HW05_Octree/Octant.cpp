#include "Octant.h"
using namespace BTX;
//  Octant
uint Octant::m_uOctantCount = 0;
uint Octant::m_uMaxLevel = 3;
uint Octant::m_uIdealEntityCount = 5;
Octant::Octant(uint a_nMaxLevel, uint a_nIdealEntityCount)
{
	/*
	* This constructor is meant to be used ONLY on the root node, there is already a working constructor
	* that will take a size and a center to create a new space
	*/
	Init();//Init the default values
	m_uOctantCount = 0;
	m_uMaxLevel = a_nMaxLevel;
	m_uIdealEntityCount = a_nIdealEntityCount;
	m_uID = m_uOctantCount;

	m_pRoot = this;
	m_lChild.clear();

	//create a rigid body that encloses all the objects in this octant, it necessary you will need
	//to subdivide the octant based on how many objects are in it already an how many you IDEALLY
	//want in it, remember each subdivision will create 8 children for this octant but not all children
	//of those children will have children of their own

	//The following is a made-up size, you need to make sure it is measuring all the object boxes in the world
	std::vector<vector3> lMinMax;
	lMinMax.push_back(vector3(-50.0f));
	lMinMax.push_back(vector3(25.0f));
	RigidBody pRigidBody = RigidBody(lMinMax);


	//The following will set up the values of the octant, make sure the are right, the rigid body at start
	//is NOT fine, it has made-up values
	m_fSize = pRigidBody.GetHalfWidth().x * 2.0f;
	
	m_v3Min = pRigidBody.GetMinGlobal();
	m_v3Max = pRigidBody.GetMaxGlobal();
	
	m_EntityList.push_back(0);

	for (uint i = 1; i < m_pEntityMngr->GetEntityCount(); ++i)
	{
		pRigidBody = *(m_pEntityMngr->GetEntity(i)->GetRigidBody());
		vector3 v3TempMax = pRigidBody.GetMaxGlobal();
		vector3 v3TempMin = pRigidBody.GetMinGlobal();

		
		if (m_v3Max.x < v3TempMax.x) 
		{
			m_v3Max.x = v3TempMax.x;
		}
			
		if (m_v3Max.y < v3TempMax.y) 
		{
			m_v3Max.y = v3TempMax.y;
		}
			
		if (m_v3Max.z < v3TempMax.z) 
		{
			m_v3Max.z = v3TempMax.z;
		}
			

		
		if (m_v3Min.x > v3TempMin.x)
		{
			m_v3Min.x = v3TempMin.x;
		}
			
		if (m_v3Min.y > v3TempMin.y)
		{
			m_v3Min.y = v3TempMin.y;
		}
			
		if (m_v3Min.z > v3TempMin.z)
		{
			m_v3Min.z = v3TempMin.z;
		}
			

		
		m_EntityList.push_back(i);
	}

	m_v3Center = 0.5f * (m_v3Max + m_v3Min);
	vector3 v3MaxDiff = m_v3Max - m_v3Min;
	m_fSize = ((v3MaxDiff.x > v3MaxDiff.y ? v3MaxDiff.x : v3MaxDiff.y) > v3MaxDiff.z ? v3MaxDiff.y : v3MaxDiff.z);
	
	float fHalfSize = m_fSize / 2.0f;

	m_v3Min = vector3(m_v3Center.x - fHalfSize, m_v3Center.y - fHalfSize, m_v3Center.z - fHalfSize);
	m_v3Max = vector3(m_v3Center.x + fHalfSize, m_v3Center.y + fHalfSize, m_v3Center.z + fHalfSize);



	if (m_EntityList.size() > m_uIdealEntityCount && m_uLevel < m_uMaxLevel) 
	{
		Subdivide();
	}
		

	m_uOctantCount++; //When we add an octant we increment the count
	ConstructTree(m_uMaxLevel); //Construct the children


}

bool Octant::IsColliding(uint a_uRBIndex)
{
	if (std::find(m_EntityList.begin(), m_EntityList.end(), a_uRBIndex) == m_EntityList.end())
	{
		return false;
	}
		
	else if (m_uChildren != 0)
	{
		bool bCollisionOverall = false;
		for (uint i = 0; i < m_lChild.size(); ++i) 
		{
			if (std::find(m_lChild[i]->m_EntityList.begin(), m_lChild[i]->m_EntityList.end(), a_uRBIndex) != m_lChild[i]->m_EntityList.end()) 
			{
				bCollisionOverall = bCollisionOverall || m_lChild[i]->IsColliding(a_uRBIndex);
			}
				
		}
			

		return bCollisionOverall;
	}
	else // base case, once we get to the leaves
	{
		bool bCollisionLeaf = false;
		Entity* collEntity = m_pEntityMngr->GetEntity(a_uRBIndex);
		for (uint i = 0; i < m_EntityList.size(); ++i)
		{
			bCollisionLeaf = bCollisionLeaf ||
				(a_uRBIndex != m_EntityList[i] && collEntity->IsColliding(m_pEntityMngr->GetEntity(m_EntityList[i])));
		}
		return bCollisionLeaf;
	}
}
void Octant::Display(uint a_nIndex, vector3 a_v3Color)
{
	if (a_nIndex == -1) 
	{
		Display(a_v3Color);
	}
	else 
	{
		if (a_nIndex == m_uID) 
		{
			m_pModelMngr->AddWireCubeToRenderList(glm::translate(m_v3Center) * glm::scale(vector3(m_fSize)), a_v3Color);
		}
		else
		{
			for (uint i = 0; i < m_uChildren; i++) 
			{
				m_pChild[i]->Display(a_nIndex);
			}
		}
	}
}
void Octant::Display(vector3 a_v3Color)
{
	m_pModelMngr->AddWireCubeToRenderList(glm::translate(m_v3Center) * glm::scale(vector3(m_fSize)), a_v3Color);

	if(!IsLeaf())
	{
		for (uint i = 0; i < 8; ++i) 
		{
			m_pChild[i]->Display();
		}
	}
}
void Octant::Subdivide(void)
{

	float size = m_fSize / 2.0f;
	//If this node has reach the maximum depth return without changes
	if (m_uLevel >= m_uMaxLevel) 
	{
		return;
	}
		
	
	//If this node has been already subdivided return without changes
	if (m_uChildren != 0) 
	{
		return;
	}
		

	//Subdivide the space and allocate 8 children

	if (m_uChildren == 8)
	{
		return;
	}

	//Corners
	vector3 v3Corners[8] = {m_v3Min, vector3(m_v3Max.x, m_v3Min.y, m_v3Min.z), vector3(m_v3Max.x, m_v3Min.y, m_v3Max.z), vector3(m_v3Min.x, m_v3Min.y, m_v3Max.z), vector3(m_v3Min.x, m_v3Max.y, m_v3Max.z), vector3(m_v3Min.x, m_v3Max.y, m_v3Min.z),vector3(m_v3Max.x, m_v3Max.y, m_v3Min.z), m_v3Max};

	for (int i = 0; i < 8; i++) 
	{
		m_pChild[i] = new Octant(m_v3Center - (m_v3Center - v3Corners[i]) / 2.0f, size);
	}

	m_uChildren = 8;


	for (uint i = 0; i < 8; ++i)
	{
		if (m_pChild[i]->m_EntityList.size() > 0)
		{
			m_lChild.push_back(m_pChild[i]);
			
			if (m_pChild[i]->m_EntityList.size() > m_uIdealEntityCount && m_pChild[i]->m_uLevel < m_uMaxLevel) \
			{
				m_pChild[i]->Subdivide();
			}
				
		}
	}

}
bool Octant::ContainsAtLeast(uint a_nEntities)
{
	//You need to check how many entity objects live within this octant
	
	return a_nEntities >= m_uIdealEntityCount; 
}
void Octant::AssignIDtoEntity(void)
{
	//Recursive method
	//Have to traverse the tree and make sure to tell the entity manager
	//what octant (space) each object is at
	if (!IsLeaf()) 
	{
		for(uint i = 0; i < 8; i++) 
		{
			if (m_pChild[i] != nullptr)
			{
				m_pChild[i]->AssignIDtoEntity();
			}
		}
	}
	else 
	{
		for (uint i = 0; i < m_pEntityMngr->GetEntityCount(); i++)
		{
			if (IsColliding(i)) 
			{
				m_pEntityMngr->AddDimension(i, m_uID);
			}
			
		}
	}

	
}
//-------------------------------------------------------------------------------------------------------------------
// You can assume the following is fine and does not need changes, you may add onto it but the code is fine as is
// in the proposed solution.
void Octant::Init(void)
{
	m_uChildren = 0;

	m_fSize = 0.0f;

	m_uID = m_uOctantCount;
	m_uLevel = 0;

	m_v3Center = vector3(0.0f);
	m_v3Min = vector3(0.0f);
	m_v3Max = vector3(0.0f);

	m_pModelMngr = ModelManager::GetInstance();
	m_pEntityMngr = EntityManager::GetInstance();

	m_pRoot = nullptr;
	m_pParent = nullptr;
	for (uint n = 0; n < 8; n++)
	{
		m_pChild[n] = nullptr;
	}
}
void Octant::Swap(Octant& other)
{
	std::swap(m_uChildren, other.m_uChildren);

	std::swap(m_fSize, other.m_fSize);
	std::swap(m_uID, other.m_uID);
	std::swap(m_pRoot, other.m_pRoot);
	std::swap(m_lChild, other.m_lChild);

	std::swap(m_v3Center, other.m_v3Center);
	std::swap(m_v3Min, other.m_v3Min);
	std::swap(m_v3Max, other.m_v3Max);

	m_pModelMngr = ModelManager::GetInstance();
	m_pEntityMngr = EntityManager::GetInstance();

	std::swap(m_uLevel, other.m_uLevel);
	std::swap(m_pParent, other.m_pParent);
	for (uint i = 0; i < 8; i++)
	{
		std::swap(m_pChild[i], other.m_pChild[i]);
	}
}
void Octant::Release(void)
{
	//this is a special kind of release, it will only happen for the root
	if (m_uLevel == 0)
	{
		KillBranches();
	}
	m_uChildren = 0;
	m_fSize = 0.0f;
	m_EntityList.clear();
	m_lChild.clear();
}
void Octant::ConstructTree(uint a_nMaxLevel)
{
	//If this method is tried to be applied to something else
	//other than the root, don't.
	if (m_uLevel != 0)
		return;

	m_uMaxLevel = a_nMaxLevel; //Make sure you mark which is the maximum level you are willing to reach
	m_uOctantCount = 1;// We will always start with one octant

	//If this was initialized before make sure its empty
	m_EntityList.clear();//Make sure the list of entities inside this octant is empty
	KillBranches();
	m_lChild.clear();

	//If we have more entities than those that we ideally want in this octant we subdivide it
	if (ContainsAtLeast(m_uIdealEntityCount))
	{
		Subdivide();
	}
	AssignIDtoEntity();//Add octant ID to Entities
	ConstructList();//construct the list of objects
}
//The big 3
Octant::Octant(vector3 a_v3Center, float a_fSize)
{
	//Will create the octant object based on the center and size but will not construct children
	Init();
	m_v3Center = a_v3Center;
	m_fSize = a_fSize;

	m_v3Min = m_v3Center - (vector3(m_fSize) / 2.0f);
	m_v3Max = m_v3Center + (vector3(m_fSize) / 2.0f);

	m_uOctantCount++;
}
Octant::Octant(Octant const& other)
{
	m_uChildren = other.m_uChildren;
	m_v3Center = other.m_v3Center;
	m_v3Min = other.m_v3Min;
	m_v3Max = other.m_v3Max;

	m_fSize = other.m_fSize;
	m_uID = other.m_uID;
	m_uLevel = other.m_uLevel;
	m_pParent = other.m_pParent;

	m_pRoot, other.m_pRoot;
	m_lChild, other.m_lChild;

	m_pModelMngr = ModelManager::GetInstance();
	m_pEntityMngr = EntityManager::GetInstance();

	for (uint i = 0; i < 8; i++)
	{
		m_pChild[i] = other.m_pChild[i];
	}
}
Octant& Octant::operator=(Octant const& other)
{
	if (this != &other)
	{
		Release();
		Init();
		Octant temp(other);
		Swap(temp);
	}
	return *this;
}
Octant::~Octant() { Release(); };
//Accessors
float Octant::GetSize(void) { return m_fSize; }
vector3 Octant::GetCenterGlobal(void) { return m_v3Center; }
vector3 Octant::GetMinGlobal(void) { return m_v3Min; }
vector3 Octant::GetMaxGlobal(void) { return m_v3Max; }
uint Octant::GetOctantCount(void) { return m_uOctantCount; }
bool Octant::IsLeaf(void) { return m_uChildren == 0; }
Octant* Octant::GetParent(void) { return m_pParent; }
//--- other methods
Octant * Octant::GetChild(uint a_nChild)
{
	//if its asking for more than the 8th children return nullptr, as we don't have one
	if (a_nChild > 7) return nullptr;
	return m_pChild[a_nChild];
}
void Octant::KillBranches(void)
{
	/*This method has recursive behavior that is somewhat hard to explain line by line but
	it will traverse the whole tree until it reaches a node with no children and
	once it returns from it to its parent it will kill all of its children, recursively until
	it reaches back the node that called it.*/
	for (uint nIndex = 0; nIndex < m_uChildren; nIndex++)
	{
		m_pChild[nIndex]->KillBranches();
		delete m_pChild[nIndex];
		m_pChild[nIndex] = nullptr;
	}
	m_uChildren = 0;
}
void Octant::DisplayLeaves(vector3 a_v3Color)
{
	/*
	* Recursive method
	* it will traverse the tree until it find leaves and once found will draw them
	*/
	uint nLeafs = m_lChild.size(); //get how many children this will have (either 0 or 8)
	for (uint nChild = 0; nChild < nLeafs; nChild++)
	{
		m_lChild[nChild]->DisplayLeaves(a_v3Color);
	}
	//Draw the cube
	m_pModelMngr->AddWireCubeToRenderList(glm::translate(IDENTITY_M4, m_v3Center) *
		glm::scale(vector3(m_fSize)), a_v3Color);
}
void Octant::ClearEntityList(void)
{
	/*
	* Recursive method
	* It will traverse the tree until it find leaves and once found will clear its content
	*/
	for (uint nChild = 0; nChild < m_uChildren; nChild++)
	{
		m_pChild[nChild]->ClearEntityList();
	}
	m_EntityList.clear();
}
void Octant::ConstructList(void)
{
	/*
	* Recursive method
	* It will traverse the tree adding children
	*/
	for (uint nChild = 0; nChild < m_uChildren; nChild++)
	{
		m_pChild[nChild]->ConstructList();
	}
	//if we find a non-empty child add it to the tree
	if (m_EntityList.size() > 0)
	{
		m_pRoot->m_lChild.push_back(this);
	}
}
