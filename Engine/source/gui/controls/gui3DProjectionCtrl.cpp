//-----------------------------------------------------------------------------  
// Gui3DProjectionCtrl  
// Doppelganger Inc  
// Orion Elenzil 200701  
//   
// This control is meant to be merely a container for other controls.  
// What's neat is that it's easy to 'attach' this control to a point in world-space  
// or, more interestingly, to an object such as a player.  
//   
// Usage:  
// * Create the Gui3DProjectionControl - by default it will be at 0, 0, 0.  
// * You can change where it's located by setting the field "offsetWorld".  
//   - note you can specify that right in the .gui file  
// * You can attach it to any SceneObject by calling "setAttachedTo()".  
//  
// Behaviour:  
// * If you're attaching it to a player, by default it will center on the player's head.  
// * If you attach it to an object, by default it will delete itself if the object is deleted.  
// * Doesn't occlude w/r/t 3D objects.  
//  
// Console Methods:  
// * SetAttachedTo(SceneObject)  
// * GetAttachedTo()  
//  
// Params:  
// * pointWorld   - read/write point in worldspace. read-only if attached to an object.  
// * offsetObject - an offset in objectspace.                                default 0, 0, 0.  
// * offsetWorld  - an offset in worldspace.                                 default 0, 0, 0.  
// * offsetScreen - an offset in screenspace.                                default 0, 0.  
// * hAlign       - horizontal alignment. 0 = left, 1 = center, 2 = right.   default center.  
// * vAlign       - vertical   alignment. 0 = top,  1 = center, 2 = bottomt. default center.  
// * useEyePoint  - H & V usage of the eyePoint, if player object.           default 0, 1. (ie - use only the vertical component)  
// * autoDelete   - self-delete when attachedTo object is deleted.           default true.  
//  
// Todo:  
// * occlusion - hide the control when its anchor point is occluded.  
// * integrate w/ zbuffer - this would actually be a change to the whole GuiControl system.  
// * allow attaching to arbitrary nodes in a skeleton.  
// * avoid projection when the object is out of the frustum.  
//  
// oxe 20070111  
//-----------------------------------------------------------------------------  
  
#include "console/console.h"  
#include "console/consoleTypes.h"  
#include "scene/sceneObject.h"  
#include "T3D/player.h"  
#include "gui/controls/gui3DProjectionCtrl.h"  
#include "T3D/gameBase/gameConnection.h"
#include "gfx/gfxDrawUtil.h"
  
IMPLEMENT_CONOBJECT(Gui3DProjectionCtrl);

IMPLEMENT_CALLBACK(Gui3DProjectionCtrl, onObjectEntersView, void, (SceneObject* obj), (obj),
   "@brief Called when the object this control is attached to enters the view.");

IMPLEMENT_CALLBACK(Gui3DProjectionCtrl, onObjectLeavesView, void, (SceneObject* obj), (obj),
   "@brief Called when the object this control is attached to leaves the view.");

IMPLEMENT_CALLBACK(Gui3DProjectionCtrl, onProjectionBleedover, void, (S32 bleedTypes), (bleedTypes),
   "@brief Called when the object this control is attached to leaves the view.");

ImplementEnumType(Gui3DProjHorizAlignment,
   "Horizontal Alignment of the project control.\n\n"
   "@ingroup GuiControl")
{
   Gui3DProjectionCtrl::left, "left"
},
{ Gui3DProjectionCtrl::horizCenter, "center" },
{ Gui3DProjectionCtrl::right,       "right" }
EndImplementEnumType;

ImplementEnumType(Gui3DProjVertAlignment,
   "Vertical Alignment of the project control.\n\n"
   "@ingroup GuiControl")
{
   Gui3DProjectionCtrl::top, "top"
},
{ Gui3DProjectionCtrl::vertCenter, "center" },
{ Gui3DProjectionCtrl::bottom,       "bottom" }
EndImplementEnumType;

ImplementEnumType(Gui3DProjAnimMode,
   "Animation mode for the control.\n\n"
   "@ingroup GuiControl")
{ Gui3DProjectionCtrl::animNone, "None" },
{ Gui3DProjectionCtrl::animSingleBob, "SingleBob" },
{ Gui3DProjectionCtrl::animLoopBob,       "LoopBob" }
EndImplementEnumType;

static U32 gBoundsBleedLEFT = Gui3DProjectionCtrl::BLEEDLEFT;
static U32 gBoundsBleedRIGHT = Gui3DProjectionCtrl::BLEEDRIGHT;
static U32 gBoundsBleedBOTTOM = Gui3DProjectionCtrl::BLEEDBOTTOM;
static U32 gBoundsBleedTOP = Gui3DProjectionCtrl::BLEEDTOP;
//-----------------------------------------------------------------------------  
  
Gui3DProjectionCtrl::Gui3DProjectionCtrl()  
{  
   mTSCtrl           = NULL;  
   mAttachedTo       = NULL;  
   mAttachedToPlayer = NULL;  
   mAutoDelete       = true;  
   mHAlign           = horizCenter;  
   mVAlign           = vertCenter;  
   mUseEyePoint.x    = 0;  
   mUseEyePoint.y    = 1;  
  
   mPtWorld     .set(0, 0, 0);  
   mPtProj      .set(0, 0);  
   mOffsetObject.set(0, 0, 0);  
   mOffsetWorld .set(0, 0, 0);  
   mOffsetScreen.set(0, 0);
   mTargetNodeName = StringTable->EmptyString();

   mAllowOcclusion = false;
   mOnlyInView = false;
   mIsObjInView = false;
   mBoundsBleed.clear();
   mObjInview = false;
   mFrameTime = PlatformTimer::create();

   mAnimationMode = animNone;
   mAnimationPos = 0.0f;
   mAnimationSpeed = 5;
   mAnimDirScalar = Point2F::Zero;
}  
  
void Gui3DProjectionCtrl::initPersistFields()  
{
   docsURL;
   Parent::initPersistFields();  
   addGroup("3DProjection");  
   addField("pointWorld"      , TypePoint3F , Offset(mPtWorld          , Gui3DProjectionCtrl));  
   addField("offsetObject"    , TypePoint3F , Offset(mOffsetObject     , Gui3DProjectionCtrl));  
   addField("offsetWorld"     , TypePoint3F , Offset(mOffsetWorld      , Gui3DProjectionCtrl));  
   addField("offsetScreen"    , TypePoint2I , Offset(mOffsetScreen     , Gui3DProjectionCtrl));

   addField("horizAlign", TYPEID< HorizAlignment >(), Offset(mHAlign, Gui3DProjectionCtrl),
      "The horizontal resizing behavior.");
   addField("vertAlign", TYPEID< VertAlignment >(), Offset(mVAlign, Gui3DProjectionCtrl),
      "The vertical resizing behavior.");

   addField("useEyePoint"     , TypePoint2I , Offset(mUseEyePoint      , Gui3DProjectionCtrl));  
   addField("autoDelete"      , TypeBool    , Offset(mAutoDelete       , Gui3DProjectionCtrl));
   addField("targetNodeName",   TypeString,  Offset(mTargetNodeName,    Gui3DProjectionCtrl));
   addField("allowOcclusion",   TypeBool,    Offset(mAllowOcclusion,    Gui3DProjectionCtrl));
   addField("onlyInView",       TypeBool,    Offset(mOnlyInView,        Gui3DProjectionCtrl));

   addField("aninmationMode", TYPEID< AnimationMode >(), Offset(mAnimationMode, Gui3DProjectionCtrl),
      "The mode of animation for the control.");
   addField("animationScalar", TypePoint2F, Offset(mAnimDirScalar, Gui3DProjectionCtrl));
   addField("animationSpeed", TypeF32, Offset(mAnimationSpeed, Gui3DProjectionCtrl));
   endGroup("3DProjection");

   Con::addVariable("BoundsBleed::Left", TypeS32, &gBoundsBleedLEFT);
   Con::addVariable("BoundsBleed::Right", TypeS32, &gBoundsBleedRIGHT);
   Con::addVariable("BoundsBleed::Top", TypeS32, &gBoundsBleedTOP);
   Con::addVariable("BoundsBleed::Bottom", TypeS32, &gBoundsBleedBOTTOM);
}  
  
void Gui3DProjectionCtrl::onRender(Point2I offset, const RectI &updateRect)  
{
   if (mAttachedTo == nullptr)
      return;

   // Must be in a TS Control
   GuiTSCtrl* parent = dynamic_cast<GuiTSCtrl*>(getParent());
   if (!parent)
      return;

   // Must have a connection and control object
   GameConnection* conn = GameConnection::getConnectionToServer();
   if (!conn)
      return;
   GameBase* control = dynamic_cast<GameBase*>(conn->getControlObject());
   if (!control)
      return;

   // Get control camera info
   MatrixF cam;
   Point3F camPos;
   VectorF camDir;
   conn->getControlCameraTransform(0, &cam);
   cam.getColumn(3, &camPos);
   cam.getColumn(1, &camDir);

   F32 camFovCos;
   conn->getControlCameraFov(&camFovCos);
   camFovCos = mCos(mDegToRad(camFovCos) / 2);

   F32 mDistanceFade = 0.1f;

   // Visible distance info & name fading
   F32 visDistance = gClientSceneGraph->getVisibleDistance();
   F32 visDistanceSqr = visDistance * visDistance;
   F32 fadeDistance = visDistance * mDistanceFade;

   // Collision info. We're going to be running LOS tests and we
   // don't want to collide with the control object.
   static U32 losMask = TerrainObjectType | ShapeBaseObjectType | StaticObjectType;
   //control->disableCollision();

   // Target pos to test, if it's a player run the LOS to his eye
   // point, otherwise we'll grab the generic box center.
   Point3F shapePos;
   if (mAttachedToPlayer != nullptr)
   {
      if (mAttachedToPlayer != NULL && mTargetNodeName != StringTable->EmptyString())
      {
         MatrixF mat;
         mAttachedToPlayer->getNodeTransform(mTargetNodeName, MatrixF::Identity, &mat);
         mat.getColumn(3, &shapePos);
      }
   }
   else
   {
      // Use the render transform instead of the box center
      // otherwise it'll jitter.
      MatrixF srtMat = mAttachedTo->getRenderTransform();
      srtMat.getColumn(3, &shapePos);
      shapePos.z += mAttachedTo->getRenderWorldBox().len_z();
   }

   VectorF shapeDir = shapePos - camPos;

   // Test to see if it's in range
   F32 shapeDist = shapeDir.lenSquared();
   if (shapeDist == 0 || shapeDist > visDistanceSqr)
      return;
   shapeDist = mSqrt(shapeDist);

   // Test to see if it's within our viewcone, this test doesn't
   // actually match the viewport very well, should consider
   // projection and box test.
   shapeDir.normalize();
   F32 dot = mDot(shapeDir, camDir);
   /*if (dot < camFovCos)
   {
      if (mObjInview)
         onObjectLeavesView_callback(mAttachedTo);
      mObjInview = false;
      return;
   }
   else
   {
      if (!mObjInview)
         onObjectEntersView_callback(mAttachedTo);
      mObjInview = true;
   }*/
   // Test to see if it's behind something, and we want to
   // ignore anything it's mounted on when we run the LOS.
   RayInfo info;
   //mAttachedTo->disableCollision();
   SceneObject* mount = mAttachedTo->getObjectMount();
   //if (mount)
   //   mount->disableCollision();
   //bool los = !gClientContainer.castRay(camPos, shapePos, losMask, &info);
  // mAttachedTo->enableCollision();
  // if (mount)
  //    mount->enableCollision();
  // if (!los)
  //    return;

   // Project the shape pos into screen space and calculate
   // the distance opacity used to fade the labels into the
   // distance.
   Point3F projPnt;
   shapePos.z += (F32)mOffsetScreen.y;
   if (!parent->project(shapePos, &projPnt))
   {
      if (mObjInview)
         onObjectLeavesView_callback(mAttachedTo);
      mObjInview = false;

      if(mOnlyInView)
         return;
   }
   else
   {
      if (!mObjInview)
         onObjectEntersView_callback(mAttachedTo);
      mObjInview = true;
   }

   if (!mOnlyInView && (projPnt.z < 0 || projPnt.z > 1))
   {
      projPnt.x = -projPnt.x;
      projPnt.y = -projPnt.y;
   }

   F32 opacity = (shapeDist < fadeDistance) ? 1.0 :
      1.0 - (shapeDist - fadeDistance) / (visDistance - fadeDistance);

   mPtScreen = Point2I((S32)projPnt.x, (S32)projPnt.y);

   // alignment  
   Point2I offsetAlign;
   switch (mHAlign)
   {
      default:
      case horizCenter:
         offsetAlign.x = -getBounds().extent.x / 2;
         break;
      case right:
         offsetAlign.x = 0;
         break;
      case left:
         offsetAlign.x = -getBounds().extent.x;
         break;
   }

   switch (mVAlign)
   {
      default:
      case vertCenter:
         offsetAlign.y = -getBounds().extent.y / 2;
         break;
      case bottom:
         offsetAlign.y = 0;
         break;
      case top:
         offsetAlign.y = -getBounds().extent.y;
         break;
   }

   RectI bounds = getBounds();
   bounds.point = mPtScreen;
   bounds.point += offsetAlign;

   // adjust the position of the control to be within the viewport
   if (bounds.point.x < mTSCtrl->getPosition().x)
   {
      bounds.point.x = mTSCtrl->getPosition().x;
      mBoundsBleed.set(BLEEDLEFT);
   }
   if (bounds.point.y < mTSCtrl->getPosition().y)
   {
      bounds.point.y = mTSCtrl->getPosition().y;
      mBoundsBleed.set(BLEEDTOP);
   }
   if (bounds.point.x + bounds.extent.x > mTSCtrl->getExtent().x)
   {
      bounds.point.x = mTSCtrl->getExtent().x - bounds.extent.x;
      mBoundsBleed.set(BLEEDRIGHT);
   }
   if (bounds.point.y + bounds.extent.y > mTSCtrl->getExtent().y)
   {
      bounds.point.y = mTSCtrl->getExtent().y - bounds.extent.y;
      mBoundsBleed.set(BLEEDBOTTOM);
   }

   if (mFrameTime->getElapsedMs() > 16)
   {
      if (mBoundsBleed.getMask() != NULL)
         onProjectionBleedover_callback(mBoundsBleed);
      mFrameTime->reset();
   }

   setBounds(bounds);

   Parent::onRender(offset, updateRect);
}  

void Gui3DProjectionCtrl::interpolateTick(F32 delta)
{
   if (!isAwake())
      return;

   Point2I ctrlPos = getPosition();

   switch (mAnimationMode) 
   {
	   case animSingleBob:
	   {
	      F32 phase = mSin(mAnimationPos * M_PI_F * 2.0f);
	      ctrlPos += Point2I(phase * mAnimDirScalar.x, phase * mAnimDirScalar.y);
	      mAnimationPos += delta * mAnimationSpeed;
	
	      if (mAnimationPos >= 1.0f)
	         mAnimationMode = animNone; // Reset to no animation after one cycle
	   }
	   break;
	   case animLoopBob:
	   {
	      F32 phase = mSin(mAnimationPos * M_PI_F * 2.0f);
	      ctrlPos += Point2I(phase * mAnimDirScalar.x, phase * mAnimDirScalar.y);
	      mAnimationPos += delta * mAnimationSpeed;
	   }
	   break;
	   default:
	      break; // No animation
   }

   setPosition(ctrlPos);
}

bool Gui3DProjectionCtrl::onWake()  
{  
   // walk up the GUI tree until we find a GuiTSCtrl.  
  
   mTSCtrl               = NULL;  
   GuiControl* walkCtrl  = getParent();  
   AssertFatal(walkCtrl != NULL, "Gui3DProjectionCtrl::onWake() - NULL parent");  
   bool doMore           = true;  
  
   while (doMore)  
   {  
      mTSCtrl  = dynamic_cast<GuiTSCtrl*>(walkCtrl);  
      walkCtrl = walkCtrl->getParent();  
      doMore   = (mTSCtrl == NULL) && (walkCtrl != NULL);  
   }  
  
   if (!mTSCtrl)  
      Con::errorf("Gui3DProjectionCtrl::onWake() - no TSCtrl parent");  
  
   return Parent::onWake();  
}  
  
void Gui3DProjectionCtrl::onSleep()  
{  
   mTSCtrl = NULL;  
   return Parent::onSleep();  
}  
  
void Gui3DProjectionCtrl::onDeleteNotify(SimObject* obj)  
{  
   // - SimSet assumes that obj is a member of THIS, which in our case ain't true.  
   // oxe 20070116 - the following doesn't compile on GCC.  
   // SimSet::Parent::onDeleteNotify(obj);  
  
   if (!obj)  
   {  
      Con::warnf("Gui3DProjectionCtrl::onDeleteNotify - got NULL");  
      return;  
   }  
  
    if (obj != mAttachedTo)  
   {  
      if (mAttachedTo != NULL)  
         Con::warnf("Gui3DProjectionCtrl::onDeleteNotify - got unexpected object: %d vs. %d", obj->getId(), mAttachedTo->getId());  
      return;  
   }  
  
   if (mAutoDelete)  
      this->deleteObject();  
}  
  
//-----------------------------------------------------------------------------  
  
void Gui3DProjectionCtrl::setAttachedTo(SceneObject* obj)  
{  
   if (obj == mAttachedTo)  
      return;  
  
   if (mAttachedTo)  
      clearNotify(mAttachedTo);  
  
   mAttachedTo       = obj;  
   mAttachedToPlayer = dynamic_cast<Player*>(obj);
  
   if (mAttachedTo)  
      deleteNotify(mAttachedTo);  
}

Point2I Gui3DProjectionCtrl::getAttachedObjProjectPos()
{
   if(!mAttachedTo)
      return Point2I(0,0);

   Point3F shapePos = mAttachedTo->getPosition();
   shapePos.z += (F32)mOffsetScreen.y;
   Point3F projPnt;
   if (!mTSCtrl->project(shapePos, &projPnt))
   {
      if(mOnlyInView)
         return Point2I(0,0);
   }

   return Point2I((S32)projPnt.x, (S32)projPnt.y);
}

void Gui3DProjectionCtrl::playAnimation()
{
   mAnimationPos = 0.0f;
}

DefineEngineMethod(Gui3DProjectionCtrl, setAttachedTo, void, (SceneObject* target), (nullAsType<SceneObject*>()), "(object)")
{
   if(target)
      object->setAttachedTo(target);
}  
  
DefineEngineMethod(Gui3DProjectionCtrl, getAttachedTo, S32, (),, "()")  
{  
   SceneObject* obj = object->getAttachedTo();  
   if (!obj)  
      return 0;  
   else  
      return obj->getId();  
}

DefineEngineMethod(Gui3DProjectionCtrl, setAttachedToByID, void, (S32 mTargetId), (0), "(object)")
{
   SceneObject* target = NULL;

   // Must have a connection and player control object
   GameConnection* conn = GameConnection::getConnectionToServer();
   if (!conn)
      return;

   if (mTargetId != -1)
   {
      target = dynamic_cast<SceneObject*>(conn->resolveGhost(mTargetId));
   }

   if (target)
      object->setAttachedTo(target);
}

DefineEngineMethod(Gui3DProjectionCtrl, getAttachedObjProjectPos, Point2I, (), , "")
{
   return object->getAttachedObjProjectPos();
}


DefineEngineMethod(Gui3DProjectionCtrl, playAnimation, void, (), , "")
{
   return object->playAnimation();
}

