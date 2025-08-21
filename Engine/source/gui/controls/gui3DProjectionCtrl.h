//-----------------------------------------------------------------------------  
// Gui3DProjectionCtrl  
// Doppelganger Inc  
// Orion Elenzil 200701  
//  
//   
//-----------------------------------------------------------------------------  
  
#ifndef _GUI3DPROJECTIONCTRL_H_  
#define _GUI3DPROJECTIONCTRL_H_  
  
#include "gui/core/guiTypes.h"  
#include "gui/core/guiControl.h"  
#include "gui/3d/guiTSControl.h"  
#include "scene/sceneObject.h"  
#include "T3D/player.h"  
  
class Gui3DProjectionCtrl : public GuiControl, public ITickable  
{  
//-----------------------------------------------------------------------------  
// stock stuff  
public:  
   Gui3DProjectionCtrl();  
   typedef GuiControl Parent;  
  
   DECLARE_CONOBJECT(Gui3DProjectionCtrl);  
  
   static  void initPersistFields     ();  
  
//-----------------------------------------------------------------------------  
// more interesting stuff  
   enum BoundsBleed
   {
      BLEEDLEFT = BIT(0),
      BLEEDRIGHT = BIT(1),
      BLEEDBOTTOM = BIT(2),
      BLEEDTOP = BIT(3)
   };
   BitSet32 mBoundsBleed;
   PlatformTimer* mFrameTime;
  
   GuiTSCtrl*                mTSCtrl;           /// must be a child of one of these.  
   SimObjectPtr<SceneObject> mAttachedTo;       /// optional object we're attached to.  
   SimObjectPtr<ShapeBase>      mAttachedToPlayer; /// same pointer as mAttachedTo, but conveniently casted to player.  
  
   Point3F                   mPtWorld;          /// the worldspace point which we're projecting  
   Point2I                   mPtProj;           /// the screenspace projected point. - note there are further modifiers before   
   Point2I                   mPtScreen;  
  
   Point3F                   mOffsetObject;     /// object-space offset applied first  to the attached point to obtain mPtWorld.  
   Point3F                   mOffsetWorld;      /// world-space  offset applied second to the attached point to obtain mPtWorld.  
   Point2I                   mOffsetScreen;     /// screen-space offset applied to mPtProj. note we still have centering, etc.

   StringTableEntry          mTargetNodeName;   /// When attached to a player, will use this node for the base transform

   bool                      mAllowOcclusion;  /// if true, we'll allow the projection to be occluded by other objects.
   bool                      mOnlyInView;     /// if true, we'll only render when the targeted object is in view.
   bool                      mIsObjInView;     /// Used for tracking if the object enters or exits the view
  
   /*enum alignment  
   {  
      min    = 0,  
      center = 1,  
      max    = 2  
   };*/

   enum HorizAlignment
   {
      left = 0,
      horizCenter = 1,
      right = 2
   };

   enum VertAlignment
   {
      top = 0,
      vertCenter = 1,
      bottom = 2
   };

  
   HorizAlignment                 mHAlign;           /// horizontal alignment  
   VertAlignment                 mVAlign;           /// horizontal alignment  
  
   bool                      mAutoDelete;       /// optionally self-delete when mAttachedTo is deleted.  
   Point2I                   mUseEyePoint;      /// optionally use the eye point. x != 0 -> horiz.  y != 0 -> vert.

   enum AnimationMode
   {
      animNone = 0,
      animSingleBob = 1,
      animLoopBob = 2
   };

   AnimationMode mAnimationMode; // Current animation mode
   F32 mAnimationSpeed; // Speed of the animation
   F32 mAnimationPos; // Current position of the animation
   Point2F mAnimDirScalar; // Scalar to adjust the direction of the animation
  
  
   void   onRender            (Point2I offset, const RectI &updateRect) override;  
   bool   onWake              () override;  
   void   onSleep             () override;  
   void   onDeleteNotify      (SimObject *object) override;  
  
   void           setAttachedTo       (SceneObject*  obj);  
   SceneObject*   getAttachedTo       ()                 { return mAttachedTo; }  
   void           setWorldPt          (Point3F& pt)      { mPtWorld = pt;      }  
   Point3F        getWorldPt          ()                 { return mPtWorld;    }
   bool mObjInview;
   DECLARE_CALLBACK(void, onObjectEntersView, (SceneObject* obj));
   DECLARE_CALLBACK(void, onObjectLeavesView, (SceneObject* obj));
   DECLARE_CALLBACK(void, onProjectionBleedover, (S32 bleedTypes));

   Point2I getAttachedObjProjectPos();

   void processTick() override {}
   void interpolateTick(F32 delta) override;
   void advanceTime(F32 timeDelta) override {}

   void playAnimation();
};

typedef Gui3DProjectionCtrl::HorizAlignment Gui3DProjHorizAlignment;
typedef Gui3DProjectionCtrl::VertAlignment Gui3DProjVertAlignment;
typedef Gui3DProjectionCtrl::AnimationMode Gui3DProjAnimMode;

DefineEnumType(Gui3DProjHorizAlignment);
DefineEnumType(Gui3DProjVertAlignment);
DefineEnumType(Gui3DProjAnimMode);
  
#endif //_GUI3DPROJECTIONCTRL_H_ 
