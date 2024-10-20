#include "Scene.h"

Scene::Scene(std::string name)
{
	mName = name;
}

///Loaders & Unloaders
void Scene::LoadActors()
{
	///Components
	numEntities = 3;
	for (int i = 0; i < numEntities; i++)
	{
		std::shared_ptr<Entity> entity = std::make_shared<Entity>(); 
		entity->SetId(i);

		mEntities.emplace_back(entity);

		std::shared_ptr<TransformComponent> transformComponent_1 = std::make_shared<TransformComponent>(); 
		transformComponent_1->m_pos.emplace_back(glm::vec3(5, 3, 1 ));
		transformComponent_1->_Size = 2;
		   
		std::shared_ptr<HealthComponent> healthComponent_1 = std::make_shared<HealthComponent>();
		healthComponent_1->health.emplace_back(100);

		std::shared_ptr<BoxCollisionComponent> boxCollisionComponent_1 = std::make_shared<BoxCollisionComponent>();
		boxCollisionComponent_1->EnableAABB.emplace_back(true); 

		std::shared_ptr<ActorComponent> actorComponent = std::make_shared<ActorComponent>();
		actorComponent->actors.emplace_back(std::make_shared<Actor>(Mesh::CreateCube(2.0f), "G�tt"));

		actorManager->AddComponent(entity->GetId(), actorComponent); 
		AABBManager->AddComponent(entity->GetId(), boxCollisionComponent_1);  
		transformManager->AddComponent(entity->GetId(),transformComponent_1); 
		healthManager->AddComponent(entity->GetId(), healthComponent_1);
	}


	//collisionSystem->Update(mEntities); 
	actorSystem = std::make_shared<ActorSystem>(actorManager);
	actorSystem->Update(mEntities);

	
	for (const auto& [id,component] : transformManager->GetAllComponents()) 
	{
		for(const auto& it : component)
		{
			it->displayComponent(); 
			std::cout << " ID:" + std::to_string(id) << "";
			std::cout << "\n";
		}
	}

	std::cout << "\n";

	for (const auto& [id, component] : actorManager->GetAllComponents())
	{
		for (const auto& it : component)
		{
			it->displayComponent();
			for (int i = 0; i < it->actors.size(); i++)
			{
				it->actors[i]->SetTexBool(true);
			}
			std::cout << " ID:" + std::to_string(id) << "";
			std::cout << "\n"; 
		}
	}

	actorSystem->AttachToCamera(mEntities, 0);
	actorSystem->GetActor(mEntities)->isPlayer = true;
	actorSystem->GetActor(mEntities)->mEnableAABBCollision = true;

	Actor::Spawner(10, -20, 20, 2);
    for (const auto& actors : Actor::spawnVector)
	{
		actors->mEnableAABBCollision = true;
	}

	/////Player
	//uActorMap["player"] = std::make_shared<Actor>(Mesh::CreateCube(2.0f),"player"); 
	//uActorMap["player"]->SetTexBool(true);
	//uActorMap["player"]->EnablePhysics = false;
	//uActorMap["player"]->ExtrudeMesh(decrease, 2.0f); 
	//uActorMap["player"]->SetLocalPosition(glm::vec3(0, 0, 5));
	//uActorMap["player"]->mEnableAABBCollision = true; 
	//uActorMap["player"]->isPlayer = false;
	//uActorMap["player"]->mAttachToActor = false; 
	//uActorMap["player"]->mCanMove = true; 



	/////Test cube
	//uActorMap["testCube"] = std::make_shared<Actor>(Mesh::CreateCube(2.0f),"TestCube"); 
	////uActorMap["testCube"]->ExtrudeMesh(Extrude::increase, 10.0f);
	//uActorMap["testCube"]->EnablePhysics = false; 
	//uActorMap["testCube"]->mEnableAABBCollision = true; 

	
	///Create camera object
    mSceneCamera = std::make_shared<Camera>("SceneCamera");
	mSceneCamera->SetLocalPosition(glm::vec3(0, -10.0f, 10.f));

	

}

void Scene::LoadContent()
{
	LoadActors();

	mShader = std::make_shared<Shader>("Shaders/Triangle.vs", "Shaders/Triangle.fs");
	mTexture = std::make_shared<Texture>("Shaders/wall.jpg",mShader);     
	
	for (const std::pair<std::string,std::shared_ptr<Actor>> &actor : uActorMap) { actor.second->SetShader(mShader); }
	for (const auto& actor : Actor::spawnVector) { actor->SetShader(mShader); }
	actorSystem->SetShader(mEntities,mShader);
	
}

void Scene::UnloadContent()
{
	//for (const std::pair<std::string, std::shared_ptr<Actor>> actor : uActorMap)
	//{
	//	if (actor.second != nullptr)
	//	{
	//		actor.second->~Actor();
	//	}
	//}

	actorSystem->GetActor(mEntities)->~Actor();

	mSceneCamera = nullptr;

	mShader = nullptr;

	mTexture->~Texture(); 

	mTexture = nullptr; 
	
	uActorMap.clear();
}

///Updater
void Scene::UpdateScene(float dt)
{
	//Camera Update
	mSceneCamera->UpdateCamera(dt); 
	BindCamera();

	//Actor Update for Bary coords
	
	if (!uActorMap.empty())
	{
		for (const std::pair<std::string, std::shared_ptr<Actor>> actor : uActorMap) { actor.second->UpdateActors(dt); }

	}
	if (!Actor::spawnVector.empty())
	{
	  for (const auto& actor : Actor::spawnVector) { actor->UpdateActors(dt); }
	}

	actorSystem->UpdateActorEntity(mEntities, dt); 

	//Collison Update
	CollisionHandling(dt);
	SpaceManipulation();
	
}

///Rednerer
void Scene::RenderScene(float dt, Transform globaltransform)
{ 
	UpdateScene(dt);  
}

///Tranformations
void Scene::SpaceManipulation() const //Only rotation can be manipulated before call in Render. Offset needs to be set in LoacActors.
{
	///Spawn objects
	/*for (auto &object : Actor::spawnVector)
	{
		 object->mMesh->SetLocalRotation(glm::vec3((float)glfwGetTime(), (float)glfwGetTime(), (float)glfwGetTime())); 
	}*/

} 

///Shader Binder
void Scene::BindCamera() const
{
	mShader->setMat4("view", mSceneCamera->GetViewMatrix()); 
	mShader->setMat4("projection", mSceneCamera->GetProjectionMatrix());
	mShader->setVec3("viewPos", mSceneCamera->GetLocalPosition());   
}

void Scene::CollisionHandling(float dt) const
{
	//Player & spawned objects
	auto max_i = Actor::spawnVector.size();
	
	if (!Actor::spawnVector.empty())
	{
		for (const auto& object : Actor::spawnVector)
		{
			if (actorSystem->GetActor(mEntities)->isPlayer)
			{
				Collision::TrackPlayer(actorSystem->GetActor(mEntities), object, dt, 5);
			}
		}
	}
}


