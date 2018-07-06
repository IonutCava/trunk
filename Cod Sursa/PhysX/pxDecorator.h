#include "PhysX.h"

class PendingActor
{
  public:
    virtual ~PendingActor(){}
	virtual NxActorDesc createActor() = 0;
	virtual void setBody(NxBodyDesc *b) = 0;
	virtual void setName(std::string n) = 0;
	virtual void setConvexShape(NxConvexShapeDesc meshDescription) = 0;
	virtual void setTriangleShape(NxTriangleMeshShapeDesc meshDescription) = 0;
	virtual void setUseConvex(bool status) = 0;
	virtual void setPos(NxVec3 pos) = 0;
};

class ConcretePendingActor : public PendingActor
{
public:
    NxActorDesc createActor()
    {
		actorDesc.body = body;
		actorDesc.globalPose.t = position;
		actorDesc.name = name.c_str();
		if(UseConvex)
		{
		    actorDesc.shapes.push_back(&staticMeshDesc);
		}
		else
		{
		    actorDesc.shapes.push_back(&staticTMeshDesc);
		}
		return actorDesc;
    }
	void setBody(NxBodyDesc *b)
	{
	    cout << "Setting Body" << endl;
		body = b;
	}
	void setName(std::string n)
	{
		cout << "Setting Name" << endl;
		name = n;
	}
	void setConvexShape(NxConvexShapeDesc meshDescription)
	{
		cout << "We are using a Convex Shape " << endl;
		staticMeshDesc = meshDescription;
	}
	void setTriangleShape(NxTriangleMeshShapeDesc meshDescription)
	{
        cout << "We are using a Triangle Shape" << endl;
		staticTMeshDesc = meshDescription;
	}
	void setUseConvex(bool status){UseConvex = status;}
	void setPos(NxVec3 pos)
	{
		cout << "Setting position to "<< pos.x << "," << pos.y << "," << pos.z << endl;
		position = pos;
	}
private:
	NxActorDesc actorDesc;
	NxBodyDesc *body;
	NxVec3 position;
	std::string name;
	bool UseConvex;
	NxConvexShapeDesc staticMeshDesc;
	NxTriangleMeshShapeDesc staticTMeshDesc;
	
};

class DecorateActor : public PendingActor
{
  public:

    DecorateActor (PendingActor *inner) {
        m_wrappee = inner;
    }
    ~DecorateActor() {
        delete m_wrappee;
    }
	NxActorDesc createActor(){return m_wrappee->createActor();}
	void setPos(NxVec3 pos){m_wrappee->setPos(pos);}
	void setBody(NxBodyDesc *b){m_wrappee->setBody(b);}
	void setName(std::string n){m_wrappee->setName(n);}
	void setUseConvex(bool status){m_wrappee->setUseConvex(status);}
	void setTriangleShape(NxTriangleMeshShapeDesc meshDescription){m_wrappee->setTriangleShape(meshDescription);}
	void setConvexShape(NxConvexShapeDesc meshDescription){m_wrappee->setConvexShape(meshDescription);}
 private:
    PendingActor *m_wrappee;

};

class ConvexShapeDecorator : public DecorateActor
{
public:
    ConvexShapeDecorator(PendingActor *inner): DecorateActor(inner) {}
	NxActorDesc createActor()
	{
		return DecorateActor::createActor();
	}
	void setBody(NxBodyDesc *b)
	{
		DecorateActor::setBody(b);
	}
	void setName(std::string n)
	{
		DecorateActor::setName(n);
	}
	void setConvexShape(NxConvexShapeDesc meshDescription)
	{
		DecorateActor::setConvexShape(meshDescription);
		DecorateActor::setUseConvex(true);
	}
	void setTriangleShape(NxTriangleMeshShapeDesc meshDescription)
	{
		DecorateActor::setTriangleShape(meshDescription);
		DecorateActor::setUseConvex(false);
	}
	void setUseConvex(bool status)
	{
		DecorateActor::setUseConvex(status);
	}
	void setPos(NxVec3 pos)
	{
		DecorateActor::setPos(pos);
	}
};

class TriangleShapeDecorator : public DecorateActor
{
public:
    TriangleShapeDecorator(PendingActor *inner): DecorateActor(inner) {}
	NxActorDesc createActor()
	{
		return DecorateActor::createActor();
	}
	void setBody(NxBodyDesc *b)
	{
		DecorateActor::setBody(b);
	}
	void setName(std::string n)
	{
		DecorateActor::setName(n);
	}
	void setConvexShape(NxConvexShapeDesc meshDescription)
	{
		DecorateActor::setConvexShape(meshDescription);
		DecorateActor::setUseConvex(true);
	}
	void setTriangleShape(NxTriangleMeshShapeDesc meshDescription)
	{
		DecorateActor::setTriangleShape(meshDescription);
		DecorateActor::setUseConvex(false);
	}
	void setUseConvex(bool status)
	{
		DecorateActor::setUseConvex(status);
	}
	void setPos(NxVec3 pos)
	{
		DecorateActor::setPos(pos);
	}
};

class BodyDecorator : public DecorateActor
{
public:
	BodyDecorator(PendingActor *inner): DecorateActor(inner) {}
	NxActorDesc createActor()
	{
		return DecorateActor::createActor();
	}
	void setBody(NxBodyDesc *b)
	{
		DecorateActor::setBody(b);
	}
	void setName(std::string n)
	{
		DecorateActor::setName(n);
	}
	void setConvexShape(NxConvexShapeDesc meshDescription)
	{
		DecorateActor::setConvexShape(meshDescription);
		DecorateActor::setUseConvex(true);
	}
	void setTriangleShape(NxTriangleMeshShapeDesc meshDescription)
	{
		DecorateActor::setTriangleShape(meshDescription);
		DecorateActor::setUseConvex(false);
	}
	void setUseConvex(bool status)
	{
		DecorateActor::setUseConvex(status);
	}
	void setPos(NxVec3 pos)
	{
		DecorateActor::setPos(pos);
	}
};

class PositionDecorator : public DecorateActor
{
public:
    PositionDecorator(PendingActor *inner): DecorateActor(inner) {}
	NxActorDesc createActor()
	{
		return DecorateActor::createActor();
	}
	void setBody(NxBodyDesc *b)
	{
		DecorateActor::setBody(b);
	}
	void setName(std::string n)
	{
		DecorateActor::setName(n);
	}
	void setConvexShape(NxConvexShapeDesc meshDescription)
	{
		DecorateActor::setConvexShape(meshDescription);
		DecorateActor::setUseConvex(true);
	}
	void setTriangleShape(NxTriangleMeshShapeDesc meshDescription)
	{
		DecorateActor::setTriangleShape(meshDescription);
		DecorateActor::setUseConvex(false);
	}
	void setUseConvex(bool status)
	{
		DecorateActor::setUseConvex(status);
	}
	void setPos(NxVec3 pos)
	{
		DecorateActor::setPos(pos);
	}
};