#pragma once

#include "gltfviewer.h"
#include "model.h"

#include <map>
#include <memory>
#include <mutex>

namespace gltfviewer {

class Models
{
public:
  Models();
  virtual ~Models();

  gltfviewer_handle AddModel( std::unique_ptr<Model> model );
  void FreeModel( const gltfviewer_handle model_handle );

  Model* const FindModel( const gltfviewer_handle model_handle );

private:
  std::map<gltfviewer_handle, std::unique_ptr<Model>> m_models;
  mutable std::mutex m_mutex;
};

} // namespace gltfviewer