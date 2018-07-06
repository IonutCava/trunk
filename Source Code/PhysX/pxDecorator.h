#include "PhysX.h"

class PendingActor
{
  public:
    virtual ~PendingActor(){}
	virtual NxActorDesc createActor() = 0;
	virtual void setBody(NxBodyDesc *b) = 0;
	virtual void setName(const std::string& n) = 0;
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
	    Con::getInstance().printfn("Setting Body");
		body = b;
	}
	void setName(const std::string& n)
	{
		Con::getInstance().printfn("Setting Name");
		name = n;
	}
	void setConvexShape(NxConvexShapeDesc meshDescription)
	{
		Con::getInstance().printfn("We are using a Convex Shape ");
		staticMeshDesc = meshDescription;
	}
	void setTriangleShape(NxTriangleMeshShapeDesc meshDescription)
	{
        Con::getInstance().printfn("We are using a Triangle Shape");
		staticTMeshDesc = meshDescription;
	}
	void setUseConvex(bool status){UseConvex = status;}
	void setPos(NxVec3 pos)
	{
		Con::getInstance().printfn("Setting position to %f , %f, %f", pos.x , pos.y , pos.z);
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
		m_wrappee = NULL;
    }
	NxActorDesc createActor(){return m_wrappee->createActor();}
	void setPos(NxVec3 pos){m_wrappee->setPos(pos);}
	void setBody(NxBodyDesc *b){m_wrappee->setBody(b);}
	void setName(const std::string& n){m_wrappee->setName(n);}
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
	void setName(const std::string& n)
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
	void setName(const std::string& n)
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
	void setName(const std::string& n)
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
	void setName(const std::string& n)
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