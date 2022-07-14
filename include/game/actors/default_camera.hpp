#pragma once 

#include <game/actor.hpp>
#include <game/components/viewer_component.hpp>
#include <game/components/keyboardMovementComponent.hpp>
#include <utils/utils.hpp>

namespace hnll {

// forward declaration
class Hve;

class HgeCamera : public HgeActor
{
public:
  HgeCamera(Hve& hve);
  ~HgeCamera(){}

  HgeCamera(const HgeCamera &) = delete;
  HgeCamera& operator=(const HgeCamera &) = delete;
  HgeCamera(HgeCamera &&) = default;
  HgeCamera& operator=(HgeCamera &&) = default;

  inline Transform& getTransform() { return transform_m; }
  template<class V> void setTranslation(V&& vec) 
  { transform_m.translation_m = std::forward<V>(vec); }    
  template<class V> void setScale(V&& vec) 
  { transform_m.scale_m = std::forward<V>(vec); }
  template<class V> void setRotation(V&& vec) 
  { transform_m.rotation_m = std::forward<V>(vec); }  

  inline s_ptr<ViewerComponent> viewerComponent() const { return spViewerComp_m; }  

private:
  Transform transform_m {};
  s_ptr<ViewerComponent> spViewerComp_m;
};

} // namespace hnll