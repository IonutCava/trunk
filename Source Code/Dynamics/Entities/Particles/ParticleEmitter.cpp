#include "Headers/ParticleEmitter.h"
#include "Dynamics/Entities/Headers/Impostor.h"

ParticleEmitterDescriptor::ParticleEmitterDescriptor() {
   _emissionInterval = 100;
   _emissionIntervalVariance = 0;
   _velocity = 2.0f;
   _velocityVariance = 1.0f;
   _lifetime           = 0;
   _lifetimeVariance   = 0;
}

ParticleEmitter::ParticleEmitter() : SceneNode(TYPE_PARTICLE_EMITTER),
								    _drawImpostor(false),
									 _enabled(true),
									 _impostor(NULL),
									 _impostorSGN(NULL)
{
}

ParticleEmitter::~ParticleEmitter(){
}

bool ParticleEmitter::unload(){
	if(_impostor){
		_particleEmitterSGN->removeNode(_impostorSGN);
	}
	SAFE_DELETE(_impostor);
	return SceneNode::unload();
}

void ParticleEmitter::postLoad(SceneGraphNode* const sgn){
	///Hold a pointer to the trigger's location in the SceneGraph
	_particleEmitterSGN = sgn;
}

///When the SceneGraph calls the particle emitter's render function, we draw the impostor if needed
void ParticleEmitter::render(SceneGraphNode* const sgn){
	///The isInView call should stop impostor rendering if needed
	if(!_impostor){
		_impostor = New Impostor(_name);
		_impostorSGN = _particleEmitterSGN->addNode(_impostor->getDummy());
	}
	_impostor->render(_impostorSGN);
}

///The descriptor defines the particle properties
void ParticleEmitter::setDescriptor(const ParticleEmitterDescriptor& descriptor){
}

///The onDraw call will emit particles
void ParticleEmitter::onDraw(const RenderStage& currentStage){
	if(!_enabled){
		return;
	}
}

/// Pre-process particles
void ParticleEmitter::tick(){
}