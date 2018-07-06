#ifndef PHYSX_H_
#define PHYSX_H_

#include "resource.h"
#include "Importer/DVDConverter.h"
#include "NxPhysics.h"
#include "NxCooking.h"
#include "pxStream.h"
#include "Debug/pxDebugRenderer.h"
#include "Utility/Headers/Singleton.h"


class NxPhysicsSDK; 
class NxScene;
class NxVec3;

SINGLETON_BEGIN( PhysX )

private:
	PhysX(); // Constructorul clasei noaste este private, deoarece implementam
	         // clasa PhysX la fel ca si clasa Engine si anume ca Singleton
	//Variabile
	bool mErrorReport;              //Daca este setat pe "true" atunci afisam erorile aruncate de Scene Manager
	bool bDebugWireframeMode;       //Un flag pentru activarea si dezactivarea randarii actorilor in mode debug
	bool writeLock;
	F32 speedMult;                  //cat de rapid sa ruleze simularile
	NxActor *currentActor;          //Un pointer catre actorul curent selectat
	NxActor *gShape;                //Pointer folosit pentru "gatirea" unui actor
	NxActor *groundPlane;
	F32    groundPos;
	DebugRenderer	 gDebugRenderer; //Initializam un Debug Renderer global pentru instanta actuala.
    NxPhysicsSDK*    gPhysicsSDK ;   //Pointer catre SDK-ul PhysX de la Nvidia
    NxScene*         gScene;         //Pointer catre scena simularii
	NxScene*         gSceneHW;       //Adaugam support si pentru PPU
    NxVec3           gDefaultGravity;//In acest vector definim gravitatia lumii noastre virtuale
	                                 //de forma (x.dir * force,y.dir * force,z.dir * force)
	                                 //Ex: Gravitatie Pamantului: (0,-9.81f,0)
	NxVec3           defaultPosition;//Putem modifica locatia la care sunt creati actorii daca vrem :)
    F32 scale;                       //Scara la care lucram. 1.0f = 1 metru
	NxSceneDesc sceneDesc;           //Aici descriem parametrii scenei
	NxPhysicsSDKDesc desc;           //Iar aici parametrii cu care initializam SDK-ul PhysX
	NxSDKCreateError errorCode;      //Variable ce va returna codul erorii aruncata de SDK

    //Functii/Metode/Subrutine
	//Pentru instantierea unui nou actor pe baza unui model importat anterior ...
	//... trebuie sa "gatim" toate triungiurile acelui model intr-un format pe care PhysX il intelege
	//createTriangleMesh = este folosit pentru obiecte in general indiferent de forma lor
	//createConvexMesh = este folosit pentru obiecte convexe deoarece este MULT mai rapid decat createTriangleMesh
	NxTriangleMesh* createTriangleMesh(const NxStream& stream); //Functie de "gatit" meshe triangulare
	NxConvexMesh*   createConvexMesh(const NxStream& stream);   //Functie de "gatit" meshe convexe
	NxActor*        createHeightfield(const NxVec3& pos, int index, NxReal step); //Functie folosita pentru crearea terenului
	

	//Aceste metode se folosesc strict pentru randarea interna a tuturor actorilor
	void DrawSphere(NxShape *sphere);  //Functie pentru randarea sferelor
	void DrawBox(NxShape *box);        // -----------||--------- cuburilor
	void DrawLowPlane(NxShape *plane); // -----------||--------- suprafetelor plane
	void DrawObjects(NxShape *shape);  // Aceasta functie itereaza prin fiecare obiect importat si il randeaza folosind functiilor interne ale importerului de modele
	F32  UpdateTime(); //Folosim aceasta metoda pentru a putea afla cu cat sa avansam simularea

protected:
	static PhysX *PhysXWorld; //Pointer folosit pentru accesare de tip singleton. Refera instanta actuala a clasei PhysX
public:
	
	void setDebugRender(bool status){bDebugWireframeMode = status;}; //Daca dorim sa afisam meshele in format wireframe apelam aceasta functie cu parametrul "true"
	bool getDebugRender(){return bDebugWireframeMode;};              //Daca vrem sa aflam tipul actual de randare, apelam aceasta functie cu rezultat boolean
	DebugRenderer *getDebugRenderer() {return &gDebugRenderer;}
	bool getWireFrameData() {return bDebugWireframeMode;}
	
    bool InitNx();            //Functie de creare a mediului fizic
    void ExitNx();            //Si o functie de distrugere al acestuie :))
	void ResetNx();
	void setGroundPlane();
	void setGroundPos(F32 y){groundPos = y;}
	//static PhysX* getInstance(); //Alta functie folosita pentru implementarea clasei PhysX ca singleton. Aceasta functie returneaza instanta curenta a clasei
	void setParameters(F32 gravity,bool ShowErrors,F32 scale);
	void setSimSpeed(F32 mult);
	//modificam pozitia la care cream actorii
	void setActorDefaultPos(vec3 position);
	NxScene *getScene(){return gScene;}      //Returnam scena creata de instanta actuala de PhysX              
	//------------------------------------------------------------------------------------------------------------------------------------------
	//Pentru a asigura asincronicitatea, urmatoarele functii tebuiesc apelate in ordinea urmatoare:
	
	    void StartPhysics();      //Daca am reusit sa cream acest mediu, putem porni simularea
	    void ProcessInput();      //Procesam orice comanda primita de la utilizator (aka noi :D )
	    void GetPhysicsResults(); //Calculam rezultatele simularii ... si reluam :)
	
	//Acest loop trebuie apelat din callback-ul de randare
    //------------------------------------------------------------------------------------------------------------------------------------------
	//Functii pentru managementul actorilor
	NxActor* AddActor(const NxActorDescBase &actorDesc); //Functie folosita pentru adaugarea unui nou actor in scena
	bool RemoveActor(NxActor  &actor);                   //Functie folosita pentru inlaturarea unui actor din scena 
	                                                      //... destul de periculoasa avand in vedere ca totul este async si nu avem semafoare :-??
	NxActor *GetSelectedActor(){return currentActor;};                             //Aceasta functie returneaza actorul selectat de utilizator
	                                                                                //sau ultimul actor creat de aplciatie
	void SetSelectedActor(/*NxActor Object*/){/*pxWorld->currentActor = Object*/}; //Aici redirectionam pointerul "currentActor" catre obiectului specifica ca parametru;                                             
	
	bool AddShape(DVDFile *mesh,bool convex/*,string cook*/);           //Cea mai ineficienta si costisitoare functie a intregii aplicatii
																	  //Competitie directa cu LoadOBJ din modelImporter
																		//Detalii in corpul functiei
	bool AddConvexShape(DVDFile *mesh);
	bool AddTriangleShape(DVDFile *mesh);
	void CreateStack(int size);                           //Aceasta metoda creaza o piramida de "size"*cuburi
	void CreateCube(NxVec3 position,int size);            //Creaza un singur cub de dimensiunea data la pozitia data
	void CreateCube(int size);                            //Creaza un singur cub de dimensiunea data la pozitia dorita
	void CreateTower(int size);                           //Creaza un "turn" de "size"*cubure
	void CreateSphere(int size);       //Creaza o singura sfera de raza data la pozitia data
	
	//Aceasta functie se apeleaza din callback-ul de randare al OpenGL-ului si randeaza TOTI actorii
	void UpdateActors(); //ToDo: un bypass in caz de un obiect nu a fost instantiat si ca actor dar trebuie totusi randat
	
	//Folosing functia aceasta aplicam un impuls de forta "forceStrength" actorului "actor" selectat pe directia "forceDir"
	//Ex: pxWorld->ApplyForceToActor(actors[3],NxVec3(-1,0,0),300); <= Lovim actorul 4 pe -x cu 300Nm
	NxVec3 ApplyForceToActor(NxActor* actor, const NxVec3& forceDir, const NxReal forceStrength);
	bool getPhysXWriteLock(){return writeLock;}
	void setPhysXWriteLock(bool status){writeLock = status;}

	SINGLETON_END()

#endif