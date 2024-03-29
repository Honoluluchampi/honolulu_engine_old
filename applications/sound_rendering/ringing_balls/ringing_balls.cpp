// hnll
#include <game/components/audio_component.hpp>
#include <game/actor.hpp>
#include <game/engine.hpp>
#include <game/modules/physics_engine.hpp>
#include <game/components/rigid_component.hpp>
#include <geometry/bounding_volume.hpp>
#include <game/components/mesh_component.hpp>
#include <game/components/point_light_component.hpp>
#include <game/actors/default_camera.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <physics/collision_info.hpp>
#include <physics/collision_detector.hpp>

using namespace hnll;

class rigid_ball : public hnll::game::actor
{
  public:
      rigid_ball() : hnll::game::actor(){}

      static s_ptr<rigid_ball> create()
      {
          // create ball actor and its mesh
          auto ball = std::make_shared<rigid_ball>();
          auto& ball_mesh = hnll::game::engine::get_mesh_model("smooth_sphere.obj");
          auto ball_mesh_vertex_position_list = ball_mesh.get_vertex_position_list();
          auto ball_mesh_comp = hnll::game::mesh_component::create(ball, "smooth_sphere.obj");

          // create bounding_sphere
          auto bounding_sphere = hnll::geometry::bounding_volume::create_bounding_sphere
                  (hnll::geometry::bv_ctor_type::RITTER, ball_mesh_vertex_position_list);
          ball->rigid_component_ = game::rigid_component::create_from_bounding_volume(*ball, std::move(bounding_sphere));

          // register the ball to the engine
          hnll::game::engine::add_actor(ball);
          physics::collision_detector::add_rigid_component(ball->rigid_component_);
          return ball;
      };

      void assign_audio(const float pitch = 440.f)
      {
        audio_component_ = game::audio_component::create();

        // raw audio creation
        const unsigned int freq = 44100;
        const float duration = 0.05f;
        std::vector<ALshort> audio(static_cast<size_t>(freq * duration));
        for (int i = 0; i < audio.size(); i++) {
          audio[i] = std::sin(pitch * M_PI * 2.0 * i / freq) * std::numeric_limits<ALshort>::max();
        }

        audio_component_->set_raw_audio(std::move(audio));
        audio_component_->get_ready_to_play();
      }

      void init(const Eigen::Vector3d& center_point, double radius)
      {
        // init ball state
        position_ = glm::vec3{center_point.x(), center_point.y(), center_point.z()};
        set_translation(glm::vec3{center_point.x(), center_point.y(), center_point.z()});
        set_scale(glm::vec3(radius, radius, radius));
        velocity_ = {0.f, 0.f, 0.f};
      }

      void update_actor(float dt) override
      {
          position_ += velocity_ * dt;
          velocity_.y += gravity_ * dt;
          this->set_translation(position_);
      }

      // this update function is invoked if collision_detector detects collision with other component (plane in this situation)
      void re_update(const hnll::physics::collision_info& info) override
      {
          position_.y -= info.intersection_depth;
          velocity_.y = -velocity_.y * rigid_component_->get_restitution();
          if (std::abs(velocity_.y) < velocity_thresh_) velocity_.y = 0;
          else {
            audio_component_->play_sound();
            this->set_translation(position_);
          }
      }

      void set_restitution(double restitution) { rigid_component_->set_restitution(restitution); }

  private:
      glm::vec3 position_;
      glm::vec3 velocity_;
      double mass_ = 3.f;
      double velocity_thresh_ = 1.3f;
      double gravity_ = 40.f;
      s_ptr<hnll::game::rigid_component> rigid_component_;
      u_ptr<game::audio_component> audio_component_;
};

// plate is bounding box of which thickness is 0.
class rigid_plane : public hnll::game::actor
{
  public:
      rigid_plane() : actor(){}
      static s_ptr<rigid_plane> create()
      {
          auto plane = std::make_shared<rigid_plane>();
          auto& plane_mesh = hnll::game::engine::get_mesh_model("big_plane.obj");
          auto plane_mesh_vertices = plane_mesh.get_vertex_position_list();
          auto bounding_box = hnll::geometry::bounding_volume::create_aabb(plane_mesh_vertices);

          plane->rigid_component_ = game::rigid_component::create_from_bounding_volume(*plane, std::move(bounding_box));

          hnll::game::engine::add_actor(plane);
          physics::collision_detector::add_rigid_component(plane->rigid_component_);

          return plane;
      }
  private:
      s_ptr<game::rigid_component> rigid_component_;
      glm::vec3 position_;
};

class falling_ball_app : public hnll::game::engine
{
  public:
   std::vector<Eigen::Vector3d> position_list = {
       {2.1f, -4.f, 0.f},
       {0.f, -8.f, 0.f},
       {-2.1f, -12.f, 0.f}
   };
   std::vector<double> pitch_list = {
       220.f,
       440.f,
       880.f,
   };
   std::vector<double> restitution_list = {
       0.3f,
       0.5f,
       0.8f,
   };

    falling_ball_app() : hnll::game::engine("falling ball")
    {
      audio::engine::start_hae_context();

      // set camera position
      camera_up_->set_translation(glm::vec3{0.f, -5.f, -20.f});

      // add rigid ball
      for (int i = 0; i < position_list.size(); i++) {
        auto ball = rigid_ball::create();
        ball->init(position_list[i], 1.f);
        ball->set_restitution(restitution_list[i]);
        ball->assign_audio(pitch_list[i]);
        balls_.emplace_back(std::move(ball));
      }

      // add plane
      auto rigid_plane = rigid_plane::create();
      game::engine::check_and_add_shading_system<game::grid_shading_system>(hnll::utils::shading_type::GRID);
    }

    ~falling_ball_app() { audio::engine::kill_hae_context(); }

    void update_game_gui() override
    {
      ImGui::Begin("debug");

      if (ImGui::Button("restart")) {
        for(int i = 0; i < balls_.size(); i++) {
          balls_[i]->init(position_list[i], 1.f);
        }
      }
      ImGui::End();
    }

  private:
    std::vector<s_ptr<rigid_ball>> balls_;
};

int main()
{
  falling_ball_app app{};
  try { app.run(); }
  catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}