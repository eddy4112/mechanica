.. default-domain:: cpp


Bullet's Collision Algorithm
****************************

Overview
========

This document attempts to better document the Bullet physics library, much of it based direclty on
the `Bullet Wiki <http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Tutorial_Articles/>`_

The bullet collision detection algorithm proceeds similarly to most rigid body / physics simulation
engines.  This document will cover the basic Bullet collision algorithm using discrete rigid
objects. Deformable (soft body) straight-forward extension. There are a number of key objects that
comprise a Bullet simulation.

* :code:`btDiscreteDynamicsWorld`
  The top level object that performs the main time stepping and connects the other pieces. Other
  kinds of Bullet simulations such as soft body employ dynamics world objects that inherit from the
  discrete dynamics world. Users must create the following objects and pass them to the discrete
  dynamics world constructor. 


* :code:`btDispatcher` foo foo
  blas blah

* :code:`btBroadphaseInterface`

* :code:`btConstraintSolver`

* :code:`btCollisionConfiguration`

  blah
  





Concepts
========


Collision Objects
-----------------

Collision Shapes
----------------

Broad Phase Collision
---------------------
Most collision detection frameworks break up collision detection into two phases, *broadphase* and
*narrowphase* 

Narrow Phase Collision
----------------------

AABB Bounding Box Hierarchy
---------------------------
This is implemented by the btDbvtBroadphase in Bullet.
As the name suggests, this is a dynamic AABB tree. One useful feature of this broadphase is that the
structure adapts dynamically to the dimensions of the world and its contents. It is very well
optimized and a very good general purpose broadphase. It handles dynamic worlds where many objects
are in motion, and object addition and removal is faster than SAP.


Collision Pairs
---------------

Collision Manifold
------------------
In collision detection, a *manifold* defines a contact area between two objects. The set of
intersection points is referred to as the contact set or contact manifold, the latter term
appropriate when the intersection set is not a finite set but a continuum of points. For example,
the intersection set of a box sitting on a table is the set of points on a face of the box. When the
contact set is desired, I refer to this as a find-intersection query. As you would expect, in most
cases a find intersection query is more expensive than a test intersection query for a given pair of
objects.

Every actual, verified collision will result in an additional collision manifold. The collision
algorithms add a collision manifold to the dispatcher if a collision occurs. New contact manifolds
are always created by the :code:`btCollisionDispatcher::getNewManifold`, which all of the collision
algorithms call. 


btPersistentManifold is a contact point cache, it stays persistent as long as objects are
overlapping in the broadphase. Those contact points are created by the collision narrow phase. The
cache can be empty, or hold 1,2,3 or 4 points. Some collision algorithms (GJK) might only add one
point at a time. updates/refreshes old contact points, and throw them away if necessary (distance
becomes too large) reduces the cache to 4 points, when more then 4 points are added, using following
rules:

* the contact point with deepest penetration is always kept, and it tries to maximuze the area
  covered by the points
  
* note that some pairs of objects might have more then one contact manifold.


Simulation Islands
------------------


      

Collision in general has a number of phases. Project the object forward. Figure out what collided,
then adjust and compensate for the collision.


The btCollisionObjectWrapper class 'wraps' a btCollisionShape, btCollisionObject, along with a
worldTransform.

The :code:`btCollisionAlgorithm::processCollision` method as it's name implies actually processs a
collision. The :code:`resoutOut` parameter is often, but not alwasy ingnored by the
caller. Basically, the :code:`btManifoldResult` objct holds onto a btPersistentManifold object. The
collision algoritm usually sets the btManifoldResult's m_manifoldPtr to point to the persistent
manifold that's owned by the collision algorithm. 




The dispatcher, btCollisionDispatcher maintains a matrix of collision algoriths,
m_doubleDispatchContactPoints. The collision shape type is used to index this matrix and get the
collision algorithm for the appropriate pair in the :code:`btCollisionDispatcher::findAlgorithm`
method. This method creates a new collision algorithm for each potential collision. Each new
algorithm typically creates a new collision manifold from the dispatcher in the algorithm's
constructor. 

Single Step
-----------

m_dynamicsWorld->stepSimulation(deltaTime);


int	btDiscreteDynamicsWorld::stepSimulation( btScalar timeStep,int maxSubSteps, btScalar
fixedTimeStep)


The interesting stuff
::
   
   for (int i=0;i<clampedSimulationSteps;i++)
   {
      internalSingleStepSimulation(fixedTimeStep);
      synchronizeMotionStates();
   }

The list of contact manifolds is created in, and owned by the dispatcher. The dispatcher is created
before the world, and handed to the world in it's ctor.

All of the collision algorithms call :code:`btDispatcher::releaseManifold` in their destructors to
tell the dispatcher to remove this manifold.


The AABB tree gets updated in the :code:`btCollisionWorld::computeOverlappingPairs` method. This
method in turn calls :code:`btDbvtBroadphase::calculateOverlappingPairs`, which in turn calls
:code:`btDbvtBroadphase::collide`.

New broadphase collision pairs typically get created by the addOverlappingPair

The collision algorithm attached to a collision pair gets destroyed via a call to
:code:`btHashedOverlappingPairCache::cleanOverlappingPair`, and the collision pair gets destroyed
via a call to :code:`btHashedOverlappingPairCache::removeOverlappingPair`. This method gets called
when a pair of AABBs formerly in contact no longer contact each other. Hence, that's why this method
gets a dispatcher as it's last argument, as the dispatcher is needed to destroy the algorithm
attached to the collision pair.

The call sequence for updating the AAPP tree, and removing pairs that are no longer colliding is:
::
   
   btCollisionWorld::performDiscreteCollisionDetection
   btCollisionWorld::computeOverlappingPairs
   btDbvtBroadphase::calculateOverlappingPairs
   btDbvtBroadphase::collide
   btHashedOverlappingPairCache::removeOverlappingPair


Time Stepping
-------------

The :code:`btDiscreteDynamicsWorld::internalSingleStepSimulation` is the main time step method. Each
time step consists of the following phases:

* Predict unconstrained motion, 	///apply gravity, predict motion


* createPredictiveContacts(timeStep);


* performDiscreteCollisionDetection(); 	///perform collision detection

* calculateSimulationIslands();

* solveConstraints(getSolverInfo()); ///solve contact and other joint constraints
	
* integrateTransforms(timeStep); 	///integrate transforms


::
   
   void	btDiscreteDynamicsWorld::internalSingleStepSimulation(btScalar timeStep)
   {

	if(0 != m_internalPreTickCallback) {
		(*m_internalPreTickCallback)(this, timeStep);
	}

	///apply gravity, predict motion
	predictUnconstraintMotion(timeStep);

	// just grab the dispatch info
	btDispatcherInfo& dispatchInfo = getDispatchInfo();

	dispatchInfo.m_timeStep = timeStep;
	dispatchInfo.m_stepCount = 0;
	dispatchInfo.m_debugDraw = getDebugDrawer();

	createPredictiveContacts(timeStep);

	///perform collision detection
	performDiscreteCollisionDetection();

	calculateSimulationIslands();

	getSolverInfo().m_timeStep = timeStep;

	///solve contact and other joint constraints
	solveConstraints(getSolverInfo());

	///CallbackTriggers();

	///integrate transforms
	integrateTransforms(timeStep);

	///update vehicle simulation
	updateActions(timeStep);

	updateActivationState( timeStep );

	if(0 != m_internalTickCallback) {
	    ( * m_internalTickCallback) (this, timeStep);
	}
   }


::
   
   void btDiscreteDynamicsWorld::createPredictiveContacts(btScalar timeStep)
   {
      releasePredictiveContacts();
      if (m_nonStaticRigidBodies.size() > 0)
      {
         createPredictiveContactsInternal( &m_nonStaticRigidBodies[ 0 ], m_nonStaticRigidBodies.size(), timeStep );
      }
   }

.. _coldet:

Tracing Through the Collision Detection Phase
---------------------------------------------
Bullet makes extensive use of callback function, so it can be rather dificult to manually trace
through an execution pass. This section follows a discrete collision that occured between a pair of
rigid boxes. This section follows a call starting at
:code:`btDiscreteDynamicsWorld::internalSingleStepSimulation` and proceeding through the following
steps.
::

   btDiscreteDynamicsWorld::internalSingleStepSimulation
   btCollisionWorld::performDiscreteCollisionDetection
   btCollisionDispatcher::dispatchAllCollisionPairs
   btHashedOverlappingPairCache::processAllOverlappingPairs
   btCollisionPairCallback::processOverlap
   btCollisionDispatcher::defaultNearCallback
   btBoxBoxCollisionAlgorithm::processCollision

The collision detection phase occurs after the unconstrained motion has been calculated. The
:code:`btCollisionWorld::performDiscreteCollisionDetection` first updates the dynamic AABB bounding
volume tree, computes the overlapping collision pairs, then performs the dispatch collision pairs
step. 

::
   
   void	btCollisionWorld::performDiscreteCollisionDetection()
   {
	btDispatcherInfo& dispatchInfo = getDispatchInfo();

	updateAabbs();

	computeOverlappingPairs();

	btDispatcher* dispatcher = getDispatcher();
	dispatcher->dispatchAllCollisionPairs(
	   m_broadphasePairCache->getOverlappingPairCache(),dispatchInfo,m_dispatcher1);	
   }



::
   
   void	btCollisionDispatcher::dispatchAllCollisionPairs(btOverlappingPairCache* pairCache,
      const btDispatcherInfo& dispatchInfo,btDispatcher* dispatcher) 
   {
	btCollisionPairCallback	collisionCallback(dispatchInfo,this);
	pairCache->processAllOverlappingPairs(&collisionCallback,dispatcher);
   }


	
::
   
   void	btHashedOverlappingPairCache::processAllOverlappingPairs(
      btOverlapCallback* callback,btDispatcher* dispatcher)
   {
      int i;

      for (i=0;i<m_overlappingPairArray.size();)
      {
         btBroadphasePair* pair = &m_overlappingPairArray[i];
	 if (callback->processOverlap(*pair))
	 {
	    removeOverlappingPair(pair->m_pProxy0,pair->m_pProxy1,dispatcher);
	    gOverlappingPairs--;
	 } else
	 {
	    i++;
	 }
      }
   }

The btOverlapCallback defaults to btCollisionPairCallback, who's processOverlap method looks like
::
   
   virtual bool	processOverlap(btBroadphasePair& pair)
   {
      (*m_dispatcher->getNearCallback())(pair,*m_dispatcher,m_dispatchInfo);
      return false;
   }

This calls the default btCollisionDispatcher::defaultNearCallback callback. Looks like every pair in
btHashedOverlappingPairCache::m_overlappingPairArray gets processed by the collision callback.

The btCollisionDispatcher::dispatchAllCollisionPairs calls this narrowphase nearcallback for each
pair that passes the 'btCollisionDispatcher::needsCollision' test. You can customize this
nearcallback


First checks if objects really really can collide. If so, uses the dispatcher's findAlgorithm as a
double dispatch (this is the matrix of collision algorithms) to find the correct algorithm
for the colliding pair. 

::
   
   //by default, Bullet will use this near callback
   void btCollisionDispatcher::defaultNearCallback(btBroadphasePair& collisionPair,
      btCollisionDispatcher& dispatcher, const btDispatcherInfo& dispatchInfo)
   {
      btCollisionObject* colObj0 = (btCollisionObject*)collisionPair.m_pProxy0->m_clientObject;
      btCollisionObject* colObj1 = (btCollisionObject*)collisionPair.m_pProxy1->m_clientObject;

      if (dispatcher.needsCollision(colObj0,colObj1))
      {
         btCollisionObjectWrapper obj0Wrap(0,colObj0->getCollisionShape(),colObj0,colObj0->getWorldTransform(),-1,-1);
	 btCollisionObjectWrapper obj1Wrap(0,colObj1->getCollisionShape(),colObj1,colObj1->getWorldTransform(),-1,-1);

	 //dispatcher will keep algorithms persistent in the collision pair
	 if (!collisionPair.m_algorithm)
	 {
	    collisionPair.m_algorithm = dispatcher.findAlgorithm(&obj0Wrap,&obj1Wrap,0, BT_CONTACT_POINT_ALGORITHMS);
	 }

	 if (collisionPair.m_algorithm)
	 {
	    btManifoldResult contactPointResult(&obj0Wrap,&obj1Wrap);
	       
	    if (dispatchInfo.m_dispatchFunc == btDispatcherInfo::DISPATCH_DISCRETE)
	    {
	       //discrete collision detection query
	       collisionPair.m_algorithm->processCollision(&obj0Wrap,&obj1Wrap,dispatchInfo,&contactPointResult);
	    } else
	    {
	       //continuous collision detection query, time of impact (toi)
	       btScalar toi = collisionPair.m_algorithm->calculateTimeOfImpact(colObj0,colObj1,dispatchInfo,&contactPointResult);
	       if (dispatchInfo.m_timeOfImpact > toi)
	          dispatchInfo.m_timeOfImpact = toi;

	    }
	 }
      }
   }


Provides a way to check if complex collision objects really do collide with other objects, checks to
see if these objects are kinematic or static. If so, they don't need to collide, and don't get
processed above 
::
   
   bool	btCollisionDispatcher::needsCollision(const btCollisionObject* body0,const
   btCollisionObject* body1)


All of the collision algorithm concrete implementations contain a btPersistentManifold*
m_manifoldPtr; pointer. This is a pointer to a persistant manifold created and owned by the
dispatcher. The algorithm's job is to process collisions, determine the  collision points, and add
them to this persistant manifold. 

btCollisionAlgorithm is an collision interface that is compatible with the Broadphase and btDispatcher.
It is persistent over frames


The collision algorithm shares a pointer to a persistent manifold, which is created by the collision
dispatcher's getNewManifold method. The dispatcher retains a pointer to this manifold. All of the
collision algorithms call this method to get a pointer to a new shared manifold. 
::
   
   btPersistentManifold* btCollisionDispatcher::getNewManifold(const btCollisionObject*
      body0,const btCollisionObject* body1);



Bullet Callbacks and Triggers
-----------------------------
The best way to determine if collisions happened between existing objects in the world, is to
iterate over all contact manifolds. This should be done during a
[http://www.bulletphysics.com/mediawiki-1.5.8/index.php?title=Stepping_The_World simulation tick
(substep) callback], because contacts might be added and removed during several substeps of a single
stepSimulation call.


A contact manifold is a cache that contains all contact points between pairs of collision objects. A
good way is to iterate over all pairs of objects in the entire collision/dynamics world:

::
   
    //Assume world->stepSimulation or world->performDiscreteCollisionDetection has been called

    int numManifolds = world->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++)
    {
        btPersistentManifold* contactManifold =  world->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject* obA = contactManifold->getBody0();
        const btCollisionObject* obB = contactManifold->getBody1();

        int numContacts = contactManifold->getNumContacts();
        for (int j = 0; j < numContacts; j++)
        {
            btManifoldPoint& pt = contactManifold->getContactPoint(j);
            if (pt.getDistance() < 0.f)
            {
                const btVector3& ptA = pt.getPositionWorldOnA();
                const btVector3& ptB = pt.getPositionWorldOnB();
                const btVector3& normalOnB = pt.m_normalWorldOnB;
            }
        }
    }

See Bullet/Demos/CollisionInterfaceDemo for a sample implementation.


btGhostObject
^^^^^^^^^^^^^

This type of collision object will keep track of its own overlapping pairs. This is much more
efficient than iterating through everything. For this example, we'll use a btPairCachingGhostObject
since we want easy access to the pair cache of the ghost object. A regular btGhostObject can be used
for things like triggers where the details of the overlap don't matter as much.


::
   
    btManifoldArray manifoldArray; btBroadphasePairArray& pairArray =
    ghostObject->getOverlappingPairCache()->getOverlappingPairArray(); int numPairs =
    pairArray.size();

    for (int i = 0; i < numPairs; ++i) { manifoldArray.clear();

	const btBroadphasePair& pair = pairArray[i];

	btBroadphasePair* collisionPair =  dynamicsWorld->getPairCache()->findPair(
    	    pair.m_pProxy0,pair.m_pProxy1);

	if (!collisionPair) continue;

	if (collisionPair->m_algorithm)
    	    collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

	for (int j = 0; j < manifoldArray.size(); j++) {
            btPersistentManifold* manifold = manifoldArray[j];

	    bool isFirstBody = manifold->getBody0() == ghostObject;

	    btScalar direction = isFirstBody ? btScalar(-1.0) : btScalar(1.0);

	    for (int p = 0; p < manifold->getNumContacts(); ++p) {
               const btManifoldPoint& pt = manifold->getContactPoint(p);

		if (pt.getDistance() < 0.f) {
		   const btVector3& ptA = pt.getPositionWorldOnA(); const
		   btVector3& ptB = pt.getPositionWorldOnB(); const btVector3& normalOnB =
		   pt.m_normalWorldOnB;

		   // handle collisions here
		}
	    }
	}
    }            

For the ghost object to work correctly, we need to add a callback to our world.

::
   
   btDiscreteDynamicsWorld* dynamicsWorld = CreateDiscreteDynamicsWorld();
   dynamicsWorld->getPairCache()->setInternalGhostPairCallback(
   new btGhostPairCallback());


Implementations of this concept can be found in the 'Bullet\Demos\CharacterDemo' and the
btKinematicCharacterController (in the recoverFromPenetration method).


Contact Test
^^^^^^^^^^^^


Bullet 2.76 onwards let you perform an instant query on the world (btCollisionWorld or
btDiscreteDynamicsWorld) using the contactTest query. The contactTest query will peform a collision
test against all overlapping objects in the world, and produces the results using a callback. The
query object doesn't need to be part of the world. In order for an efficient query on large worlds,
it is important that the broadphase aabbTest is accelerated, for example using the btDbvtBroadphase
or btAxisSweep3 broadphase.


An advantage of this method is that you can perform collision tests at a reduced temporal resolution
if you do not need collision tests at every physics tic.  It is also convenient to use with a
pre-existing object in the world, whereas btGhostObject would require synchronizing with the target
object.  However, a downside is that collision detection is being duplicated for the target object
(if it already exists in the world), so frequent or widespread collision tests may become less
efficient than iterating over previously generated collision pairs.

::
   
   struct ContactSensorCallback : public btCollisionWorld::ContactResultCallback {

      //! Constructor, pass whatever context you want to have available when processing contacts
      /*! You may also want to set m_collisionFilterGroup and m_collisionFilterMask
       *  (supplied by the superclass) for needsCollision() */
      ContactSensorCallback(btRigidBody& tgtBody , YourContext& context /*, ... */)
         : btCollisionWorld::ContactResultCallback(), body(tgtBody), ctxt(context) { }

      btRigidBody& body; //!< The body the sensor is monitoring
      YourContext& ctxt; //!< External information for contact processing

      //! If you don't want to consider collisions where the bodies are joined by a constraint, override needsCollision:
      /*! However, if you use a btCollisionObject for #body instead of a btRigidBody,
       *  then this is unnecessary—checkCollideWithOverride isn't available */
      virtual bool needsCollision(btBroadphaseProxy* proxy) const {
         // superclass will check m_collisionFilterGroup and m_collisionFilterMask
         if(!btCollisionWorld::ContactResultCallback::needsCollision(proxy))
            return false;
         // if passed filters, may also want to avoid contacts between constraints
         return body.checkCollideWithOverride(static_cast<btCollisionObject*>(proxy->m_clientObject));
      }

      //! Called with each contact for your own processing (e.g. test if contacts fall in within sensor parameters)
      virtual btScalar addSingleResult(btManifoldPoint& cp,
      const btCollisionObjectWrapper* colObj0,int partId0,int index0,
      const btCollisionObjectWrapper* colObj1,int partId1,int index1) {
         btVector3 pt; // will be set to point of collision relative to body
         if(colObj0->m_collisionObject==&body) {
            pt = cp.m_localPointA;
         } else {
            assert(colObj1->m_collisionObject==&body && "body does not match either collision object");
            pt = cp.m_localPointB;
         }
         // do stuff with the collision point
         return 0; // There was a planned purpose for the return value of addSingleResult, but it is not used so you can ignore it.
      }
   };

   // USAGE:
   btRigidBody* tgtBody /* = ... */;
   YourContext foo;
   ContactSensorCallback callback(*tgtBody, foo);
   world->contactTest(tgtBody,callback);

Contact Pair Test
^^^^^^^^^^^^^^^^^


Bullet 2.76 onwards provides the contactPairTest to perform collision detection between two specific
collision objects only. Contact results are passed on using the provided callback. They don't need
to be inserted in the world. See btCollisionWorld::contactPairTest in
Bullet/src/BulletCollision/CollisionDispatch/btCollisionWorld.h for implementation details.


Contact Callbacks
^^^^^^^^^^^^^^^^^

Be careful when using contact callbacks: They may be called too frequently for your purposes. Bullet
supports custom callbacks at various points in the collision system. The callbacks themselves are
very simply implemented as global variables that you set to point at appropriate functions. Before
you can expect them to be called you must set an appropriate flag in your rigid body:

::
   mBody->setCollisionFlags(mBody->getCollisionFlags()
      | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

There are three collision callbacks:

gContactAddedCallback


This is called whenever a contact is added (note that the same contact may be added multiple times
before it is processed). From here, you can modify some properties (e.g. friction) of the contact
point.  '''Note:''' gContactAddedCallback does not appear to work when using multithreaded solvers.

::
   
   typedef bool (*ContactAddedCallback)(
      btManifoldPoint& cp,
      const btCollisionObject* colObj0,
      int partId0,
      int index0,
      const btCollisionObject* colObj1,
      int partId1,
      int index1);

As of the current implementation of Bullet (2.82), the return value of this function is ignored.

==gContactProcessedCallback==

This is called immediately after the collision has actually been processed.

::
   
   typedef bool (*ContactProcessedCallback)(
      btManifoldPoint& cp,
      void* body0,void* body1);

Note that ''body0'' and ''body1'' are pointers to the same btCollisionObjects as ''colObj0'' and
''colObj1'' in the ''gContactAddedCallback'' (exactly why this function prototype is declared
differently is unclear).


As of the current implementation of Bullet (2.82), the return value of this function is ignored.

``gContactDestroyedCallback``

This is called immediately after the contact point is destroyed.
::
   
   typedef bool (*ContactDestroyedCallback)(
      void* userPersistentData);

The passed ``userPersistentData`` argument is the value of the ''m_userPersistentData'' member of
the ``btManifoldPoint`` which has been destroyed (this can be set in ''gContactAddedCallback'' or
''gContactProcessedCallback'').


'''Note that gContactDestroyedCallback will not be called for any contact point unless
''cp.m_userPersistentData'' is set!'''  You must set this (to some value other than NULL) in a prior
contact added/processed callback in order to receive a destroyed callback.


Triggers
^^^^^^^^

Collision objects with a callback still have collision response with dynamic rigid bodies. In order
to use collision objects as trigger, you have to disable the collision response.


::
   
   mBody->setCollisionFlags(mBody->getCollisionFlags() |
      btCollisionObject::CF_NO_CONTACT_RESPONSE));

Triggers and the ``btKinematicCharacterController``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The stock :code:`btKinematicCharacterController` doesn't appear to properly behave with ghost
objects that have :code:`CF_NO_CONTACT_RESPONSE` set. It seems to ignore that flag and act as
if the objects don't have that flag set.


The solution is to create a custom character controller based on the btKinematicCharacterController
class, and make a few changes, as detailed in this forum post:


http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=9&t=5684

Add this IF into the method '''btKinematicCharacterController::recoverFromPenetration''', under the
<code>btBroadphasePair* collisionPair = &m_ghostObj...</code> line:

::
   
   //for trigger filtering
   if (!static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject)->hasContactResponse() ||
      !static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject)->hasContactResponse())
      continue;

And add this IF to the beginning of '''btKinematicClosestNotMeConvexResultCallback::addSingleResult''':
::
   
   //for trigger filtering
   if (!convexResult.m_hitCollisionObject->hasContactResponse())
      return btScalar(1.0);


Classes
=======

This section describes the key classes involved with Bullet collision detection.

.. _btBroadphasePair:

.. class::btBroadphasePair

https://github.com/bulletphysics/bullet3/blob/master/src/BulletCollision/BroadphaseCollision/btBroadphaseProxy.h

The ``btBroadphasePair`` object represents a pair of pair of AABB-overlapping collision objects. The
``btBroadphasePair`` exists as long as this pair of collision objects have operlappng AABB volumes. 
A ``btDispatcher`` can search a ``btCollisionAlgorithm`` that performs exact/narrowphase collision
detection on the actual collision shapes.


The ``btBroadphasePair`` holds a reference to a collision algorithm. The collision algorithm
lifetime is the same as the broadphase pair. The :code:`btBroadphasePair`

When the AABB's of a pair of collision objects overlap, the :code:`btHashedOverlappingPairCache::addOverlappingPair`, in a call to
:code:`btHashedOverlappingPairCache::internalAddPair` creates a new :code:`btBroadphasePair`. This
new object is added to, and managed by a ``btOverlappingPairCache``. The ``btBroadphasePair``'s ``m_algorithm`` pointer is initially NULL.  


The collision pair's algorithm is created in the
dispatcher's ``findAlgorithm`` method. The basic idea is that the collision algorithm exists as
long as a broadphase collision pair exists, the collision algorithm exists as long as a pair of
objects remain in AABB contact.


A simplified ``btBroadphasePair`` ::
   
   struct btBroadphasePair {
      btBroadphaseProxy* m_pProxy0;
      btBroadphaseProxy* m_pProxy1;
      mutable btCollisionAlgorithm* m_algorithm;
      //don't use this data, it will be removed in future version.
      union { void* m_internalInfo1; int m_internalTmpValue;};
   }

some more stuff

.. class:: btCollisionAlgorithm

https://github.com/bulletphysics/bullet3/blob/master/src/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h

::
   
   class btCollisionAlgorithm {
   protected:
      btDispatcher* m_dispatcher;	
   public:
      btCollisionAlgorithm() {};
      btCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci);
      virtual ~btCollisionAlgorithm() {};
      virtual void processCollision (const btCollisionObjectWrapper* body0Wrap,
         const btCollisionObjectWrapper* body1Wrap,
	 const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut) = 0;
      virtual btScalar calculateTimeOfImpact(btCollisionObject* body0,
         btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,
	 btManifoldResult* resultOut) = 0;
      virtual void getAllContactManifolds(btManifoldArray& manifoldArray) = 0;
   };

      

.. class:: btPersistentManifold


https://github.com/bulletphysics/bullet3/blob/master/src/BulletCollision/NarrowPhaseCollision/btPersistentManifold.h#L63

::
   
   class btPersistentManifold : public btTypedObject {
      btManifoldPoint m_pointCache[MANIFOLD_CACHE_SIZE];

      /// this two body pointers can point to the physics rigidbody class.
      const btCollisionObject* m_body0;
      const btCollisionObject* m_body1;

      int	m_cachedPoints;

      btScalar	m_contactBreakingThreshold;
      btScalar	m_contactProcessingThreshold;

      public:
         int	m_companionIdA;
	 int	m_companionIdB;

	 int m_index1a;

	 btPersistentManifold();

	 btPersistentManifold(const btCollisionObject* body0, const btCollisionObject*
	    body1,int , btScalar contactBreakingThreshold,
	    btScalar contactProcessingThreshold);

	const btCollisionObject* getBody0() const { return m_body0;}
	const btCollisionObject* getBody1() const { return m_body1;}

	void	setBodies(const btCollisionObject* body0,const btCollisionObject* body1);

	void clearUserCache(btManifoldPoint& pt);

	int	getNumContacts();
	
	/// the setNumContacts API is usually not used, except when you gather/fill all contacts manually
	void setNumContacts(int cachedPoints);

	const btManifoldPoint& getContactPoint(int index) const;

	btManifoldPoint& getContactPoint(int index);

	///@todo: get this margin from the current physics / collision environment
	btScalar	getContactBreakingThreshold() const;

	btScalar	getContactProcessingThreshold() const;
	
	void setContactBreakingThreshold(btScalar contactBreakingThreshold);

	void setContactProcessingThreshold(btScalar	contactProcessingThreshold);

	int getCacheEntry(const btManifoldPoint& newPoint) const;

	int addManifoldPoint( const btManifoldPoint& newPoint, bool isPredictive=false);

	void removeContactPoint (int index);
	
	void replaceContactPoint(const btManifoldPoint& newPoint,int insertIndex);

	bool validContactDistance(const btManifoldPoint& pt) const;
	
	/// calculated new worldspace coordinates and depth, and reject points that exceed the collision margin
	void	refreshContactPoints(  const btTransform& trA,const btTransform& trB);

	void	clearManifold();
   }

.. class:: btDiscreteDynamicsWorld

https://github.com/bulletphysics/bullet3/blob/master/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h

The :class:`btDiscreteDynamicsWorld` is the main way to create discrete rigid body simulations. It inherits
from ``btDynamicsWorld``, which in turn inherits from the base ``btCollisionWorld``. The
btDiscreteDynamicsWorld determines the response to the identified collisions.


::
   
   class btDiscreteDynamicsWorld : public btDynamicsWorld {
      btAlignedObjectArray<btTypedConstraint*>	     m_sortedConstraints;
      InplaceSolverIslandCallback* 	             m_solverIslandCallback;
      btConstraintSolver*	                     m_constraintSolver;
      btSimulationIslandManager*	             m_islandManager;
      btAlignedObjectArray<btTypedConstraint*>       m_constraints;
      btAlignedObjectArray<btRigidBody*>             m_nonStaticRigidBodies;
      btVector3	                                     m_gravity;

      //for variable timesteps
      btScalar	                                     m_localTime;
      btScalar	                                     m_fixedTimeStep;

      //for variable timesteps
      bool	                                     m_ownsIslandManager;
      bool	                                     m_ownsConstraintSolver;
      bool	                                     m_synchronizeAllMotionStates;
      bool	                                     m_applySpeculativeContactRestitution;
      
      btAlignedObjectArray<btActionInterface*>	     m_actions;
      int	                                     m_profileTimings;
      bool	                                     m_latencyMotionStateInterpolation;
      btAlignedObjectArray<btPersistentManifold*>    m_predictiveManifolds;
      
      // used to synchronize threads creating predictive contacts
      btSpinMutex                                    m_predictiveManifoldsMutex;  
   }


.. class:: btDynamicsWorld

The btDynamicsWorld is the interface class for several dynamics implementation, basic, discrete,
parallel, and continuous etc.

::

   class btDynamicsWorld : public btCollisionWorld {
      btInternalTickCallback                         m_internalTickCallback;
      btInternalTickCallback                         m_internalPreTickCallback;
      void*	                                     m_worldUserInfo;
      btContactSolverInfo                            m_solverInfo;
   }


.. class:: btCollisionWorld

The ``btCollisionWorld`` sets up the basic collision framework, it manages the broadphase and
dispatcher. 

::
   
   class btCollisionWorld {
      btAlignedObjectArray<btCollisionObject*>       m_collisionObjects;
      btDispatcher*	                             m_dispatcher1;
      btDispatcherInfo	                             m_dispatchInfo;
      btBroadphaseInterface*	                     m_broadphasePairCache;
      btIDebugDraw*	                             m_debugDrawer;
      
      ///m_forceUpdateAllAabbs can be set to false as an optimization to only update active object AABBs
      ///it is true by default, because it is error-prone (setting the position of static objects wouldn't update their AABB)
      bool m_forceUpdateAllAabbs;
   }




.. class:: btOverlappingPairCache : public btOverlappingPairCallback

   The ``btOverlappingPairCache`` provides an interface for overlapping pair management (add,
   remove, storage), used by the ``btBroadphaseInterface`` broadphases. Pair caches manage pairs of
   collision objects who's AABB volumes overlap. The
   ``btHashedOverlappingPairCache`` and ``btSortedOverlappingPairCache classes`` are two
   implementations. 
