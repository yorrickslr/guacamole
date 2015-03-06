// class header
#include <gua/utils/SkinnedMesh.hpp>
#include <gua/utils/Timer.hpp>

//external headers
#include <iostream>
#include <queue>
#include <map>

namespace gua{

SkinnedMesh::SkinnedMesh():
 positions{},
 normals{},
 texCoords{},
 tangents{},
 bitangents{},
 indices{}
{}

SkinnedMesh::SkinnedMesh(FbxMesh& mesh, Bone const& root) {
  Timer timer{};
  timer.start();

  num_vertices = mesh.GetControlPointsCount(); 
  std::string UV_set;
  bool has_uvs = true;

//UV coordinates
  if(mesh.GetElementUVCount() == 0) {
    Logger::LOG_WARNING << "Mesh has no texture coordinates" << std::endl;
    has_uvs = false;
  }
  else {
    if(mesh.GetElementUVCount() > 1) {
      Logger::LOG_WARNING << "Mesh has multiple UV sets, only using first one" << std::endl;
    }
    UV_set = mesh.GetElementUV(0)->GetName();
  }

  FbxVector4 translation = mesh.GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot);
  FbxVector4 rotation = mesh.GetNode()->GetGeometricRotation(FbxNode::eSourcePivot);
  FbxVector4 scaling = mesh.GetNode()->GetGeometricScaling(FbxNode::eSourcePivot);
  FbxAMatrix geo_transform(translation, rotation, scaling);

  FbxAMatrix identity = FbxAMatrix{};
  identity.SetIdentity();
  if(geo_transform != identity) {
    Logger::LOG_WARNING << "Mesh has Geometric Transform, vertices will be skewed." << std::endl;
  }

//normals
  if(mesh.GetElementNormalCount() == 0) {
    //dont override exiting normals and generate by control point, not vertex
    mesh.GenerateNormals(false, true);
  }

//tangents
  bool has_tangents = true;
  if(mesh.GetElementTangentCount() == 0 || mesh.GetElementBinormalCount() == 0) {
    if(has_uvs) { 
      Logger::LOG_DEBUG << "Generating tangents" << std::endl;
      mesh.GenerateTangentsData(UV_set.c_str(), true);
    }
    else {
      Logger::LOG_DEBUG << "No UVs, can't generate tangents" << std::endl;
      has_tangents = false;
    }
  }

//polygons
  if(mesh.GetPolygonCount() < 1) {
    Logger::LOG_ERROR << "No polygons in mesh" << std::endl;
    assert(0);
  }

  //struct to save info about future vertex
  struct temp_vert {
    temp_vert(unsigned oindex, unsigned pt, scm::math::vec3f norm, unsigned tr, unsigned ind):
     old_index{oindex},
     point{pt},
     normal{norm},
     uv{},
     tangent{},
     bitangent{},
     tris{}
    {
      tris.push_back(std::make_pair(tr, ind));
    }
    unsigned old_index;
    unsigned point;
    scm::math::vec3f normal;
    scm::math::vec3f tangent;
    scm::math::vec3f bitangent;
    scm::math::vec2f uv;
    std::vector<std::pair<unsigned, unsigned>> tris; //tris which share vertex
  };
  //struct to save info about future triangle
  struct temp_tri {
    temp_tri(unsigned a, unsigned b, unsigned c):
     verts{a, b, c}
    {}
    std::array<unsigned, 3> verts;
  };
  //one vector of temp_vert represents one control point, every temp_vert in that vector is one vertex at that point
  std::vector<std::vector<temp_vert>> vert_positions{unsigned(mesh.GetControlPointsCount()), std::vector<temp_vert>{}};
  std::vector<temp_tri> temp_tris{};
  
  //this function gets a geometry layer and returns the function to access it depending on mapping & referencing
  auto get_access_function = [](FbxLayerElementTemplate<FbxVector4> const& layer) {
    std::function<unsigned(temp_vert const&)> access_function;
    //mapping to control point
    if(layer.GetMappingMode() == FbxGeometryElement::eByControlPoint) {
      if(layer.GetReferenceMode() == FbxGeometryElement::eDirect) {
        access_function =[](temp_vert const& vert)->unsigned {
          return vert.point;
        };
      }
      else if(layer.GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
        access_function = [&layer](temp_vert const& vert)->unsigned {
          return layer.GetIndexArray().GetAt(vert.point);
        };
      }
      else {
        Logger::LOG_ERROR << "Type of reference not supported" << std::endl;
      }
    }
    //mapping to vertex
    else if(layer.GetMappingMode() == FbxGeometryElement::eByPolygonVertex){
      if(layer.GetReferenceMode() == FbxGeometryElement::eDirect) {
        access_function =[](temp_vert const& vert)->unsigned {
          return vert.old_index;
        };
      }
      else if(layer.GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
        access_function = [&layer](temp_vert const& vert)->unsigned {
          return layer.GetIndexArray().GetAt(vert.old_index);
        };
      }
      else {
        Logger::LOG_ERROR << "Type of reference not supported" << std::endl;
      }
    }
    else {
      Logger::LOG_ERROR << "Type of mapping not supported" << std::endl;
    }

    return access_function;
  };


  FbxVector4* control_points = mesh.GetControlPoints();
  //vertex indices of polygons
  int* poly_vertices = mesh.GetPolygonVertices();

  FbxArray<FbxVector4> poly_normals;
  mesh.GetPolygonVertexNormals(poly_normals);

  FbxArray<FbxVector2> poly_uvs{};
  if(has_uvs) {
    mesh.GetPolygonVertexUVs(UV_set.c_str(), poly_uvs);
  }

  //define function to access tangents and bitangents
  std::function<unsigned(temp_vert const&)> get_tangent;
  std::function<unsigned(temp_vert const&)> get_bitangent;

  FbxArray<FbxVector4> poly_tangents;
  FbxArray<FbxVector4> poly_bitangents;

 if(has_tangents){
    FbxGeometryElementTangent* tangent_info = mesh.GetElementTangent(0);
    tangent_info->GetDirectArray().CopyTo(poly_tangents);
    get_tangent = get_access_function(*tangent_info);
    
    FbxGeometryElementBinormal* bitangent_info = mesh.GetElementBinormal(0);
    bitangent_info->GetDirectArray().CopyTo(poly_bitangents);
    get_bitangent = get_access_function(*bitangent_info);
  }

  num_triangles = 0; 
  //starting index of the polygon in the index array
  unsigned start_index = 0;
  //how many tris the last polygon contained
  unsigned tris_added = 0;
  //iterate over polygons
  for(unsigned i = 0; i < mesh.GetPolygonCount(); ++i)
  {
    //triangulate face if necessary
    for(unsigned j = 2; j < mesh.GetPolygonSize(i); ++j)
    {
      //get indices of vertices in attribute arrays
      std::array<unsigned, 3> indices{start_index, start_index + j - 1, start_index + j};
      //create triangle from vertex indices
      temp_tri tri{unsigned(poly_vertices[indices[0]]), unsigned(poly_vertices[indices[1]]), unsigned(poly_vertices[indices[2]])};
      temp_tris.push_back(tri);

      //get references to the control points at which the vertices lie
      std::vector<temp_vert>& curr_point1 = vert_positions[tri.verts[0]];
      std::vector<temp_vert>& curr_point2 = vert_positions[tri.verts[1]];
      std::vector<temp_vert>& curr_point3 = vert_positions[tri.verts[2]];
      //add the new vertices to the control points
      curr_point1.push_back(temp_vert{indices[0], tri.verts[0], to_gua::vec3(poly_normals[indices[0]]), num_triangles, 0});
      curr_point2.push_back(temp_vert{indices[1], tri.verts[1], to_gua::vec3(poly_normals[indices[1]]), num_triangles, 1});
      curr_point3.push_back(temp_vert{indices[2], tri.verts[2], to_gua::vec3(poly_normals[indices[2]]), num_triangles, 2});
      
      //set optional data
      if(has_uvs) {
        curr_point1[curr_point1.size()-1].uv = to_gua::vec2(poly_uvs[indices[0]]);
        curr_point2[curr_point2.size()-1].uv = to_gua::vec2(poly_uvs[indices[1]]);
        curr_point3[curr_point3.size()-1].uv = to_gua::vec2(poly_uvs[indices[2]]); 
      }
      if(has_tangents) {
        curr_point1[curr_point1.size()-1].tangent = to_gua::vec3(poly_tangents[get_tangent(curr_point1[curr_point1.size()-1])]);
        curr_point1[curr_point1.size()-1].bitangent = to_gua::vec3(poly_bitangents[get_bitangent(curr_point1[curr_point1.size()-1])]);
        
        curr_point2[curr_point2.size()-1].tangent = to_gua::vec3(poly_tangents[get_tangent(curr_point2[curr_point2.size()-1])]);
        curr_point2[curr_point2.size()-1].bitangent = to_gua::vec3(poly_bitangents[get_bitangent(curr_point2[curr_point2.size()-1])]);
        
        curr_point3[curr_point3.size()-1].tangent = to_gua::vec3(poly_tangents[get_tangent(curr_point3[curr_point3.size()-1])]);
        curr_point3[curr_point3.size()-1].bitangent = to_gua::vec3(poly_bitangents[get_bitangent(curr_point3[curr_point3.size()-1])]);
      }
      ++tris_added;
      ++num_triangles;
    }
    start_index += 2 + tris_added;
    tris_added = 0;
  }

  //filter out duplicate vertices
  num_vertices = 0;
  unsigned old_num_vertices = 0;
  unsigned dupl_verts = 0;
  //iterate over control points
  for(std::vector<temp_vert>& verts : vert_positions) {
    old_num_vertices += verts.size();
    //iterate over vertices at that point
    for(auto iter = verts.begin(); iter != verts.end(); ++iter) {
      //ierate over vertices behind current vertex
      for(std::vector<temp_vert>::iterator iter2 = std::next(iter); iter2 != verts.end(); ++iter2) {
        //match by normals and if exisiting, other attributes
        bool duplicate = iter2->normal == iter->normal;
        if(duplicate && has_uvs) duplicate = iter2->uv == iter->uv; 
        if(duplicate && has_tangents) duplicate = iter2->tangent == iter->tangent && iter2->bitangent == iter->bitangent; 
        //duplicate -> merge vertices
        if(duplicate) {
          //add triangle of duplicate vertex to current vertex
          iter->tris.push_back(iter2->tris[0]);

          verts.erase(iter2);
          ++dupl_verts;
          break;
        }
      }
    }
    //add number of filtered verts at current control point to total vertex number
    num_vertices += verts.size();
  }
  Logger::LOG_DEBUG << dupl_verts << " vertex duplications" << std::endl;

  bool has_weights = false;
  std::vector<bone_influences> ctrlpt_weights{};
  //get weights of original control points
  if(root.name != "none") {
    ctrlpt_weights = get_weights(mesh, root);
    has_weights = ctrlpt_weights.size() > 0;
  }

  // Reserve space in the vectors for the vertex attributes and indices
  positions.reserve(num_vertices);
  normals.reserve(num_vertices);
  if(has_uvs) {
    texCoords.reserve(num_vertices);
  }
  else {
    texCoords.resize(num_vertices, scm::math::vec2f(0.0f));
  }
  if(has_tangents) {
    tangents.reserve(num_vertices);
  }
  else {
    tangents.resize(num_vertices, scm::math::vec3f(0.0f));
    bitangents.resize(num_vertices, scm::math::vec3f(0.0f));
  }

  //load reduced attributes
  unsigned curr_vert = 0;
  //iterate over control points
  for(std::vector<temp_vert> const& verts : vert_positions) {
    //iterate over vertices at that point
    for(temp_vert const& vert : verts) {
      //update containing triangles with actual index of this vertex in member vectors
      for(auto const& tri : vert.tris) {
        temp_tris[tri.first].verts[tri.second] = curr_vert;
      }
      //push properties to attribute vectors
      positions.push_back(to_gua::vec3(control_points[vert.point]));
      normals.push_back(vert.normal);

      if(has_tangents) {
        tangents.push_back(vert.tangent);
        bitangents.push_back(vert.bitangent);
      }
      if(has_uvs) {
        texCoords.push_back(vert.uv);
      }
      if(has_weights) {
        bone_influences const& curr_influence{ctrlpt_weights[vert.point]};
        //push all bone influences for current vert
        for(uint i(0); i < curr_influence.weights.size(); ++i){
          bone_ids.push_back(curr_influence.IDs[i]);
          bone_weights.push_back(curr_influence.weights[i]);
        }

        bone_counts.push_back(curr_influence.weights.size());
      }
      ++curr_vert;
    }
  }

  //load reduced triangles
  indices.reserve(num_triangles * 3);
  for(temp_tri const& tri : temp_tris) {
    indices.push_back(tri.verts[0]);
    indices.push_back(tri.verts[1]);
    indices.push_back(tri.verts[2]);
  }

  //output reduction info
  Logger::LOG_DEBUG << "Number of vertices reduced from " << old_num_vertices << " to " << num_vertices << " ,time taken: " << timer.get_elapsed() << std::endl;
}

SkinnedMesh::SkinnedMesh(aiMesh const& mesh, Bone const& root) {   
  num_triangles = mesh.mNumFaces;
  num_vertices = mesh.mNumVertices; 
  
  // Reserve space in the vectors for the vertex attributes and indices
  positions.reserve(num_vertices);
  normals.reserve(num_vertices);
  texCoords.reserve(num_vertices);
  tangents.reserve(num_vertices);
  bitangents.reserve(num_vertices);
  indices.reserve(num_triangles * 3);


  // Populate the vertex attribute vectors
  for (uint i = 0 ; i < mesh.mNumVertices ; i++) {
    
    scm::math::vec3f pPos = scm::math::vec3f(0.0f);
    if(mesh.HasPositions()) {
      pPos = to_gua::vec3(mesh.mVertices[i]);
    }

    scm::math::vec3f pNormal = scm::math::vec3f(0.0f);
    if(mesh.HasNormals()) {
      pNormal = to_gua::vec3(mesh.mNormals[i]);
    }

    scm::math::vec2f pTexCoord = scm::math::vec2(0.0f);
    if(mesh.HasTextureCoords(0)) {
      pTexCoord = scm::math::vec2(mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y);
    }

    scm::math::vec3f pTangent = scm::math::vec3f(0.0f);
    scm::math::vec3f pBitangent = scm::math::vec3f(0.0f);
    if (mesh.HasTangentsAndBitangents()) {
      pTangent = to_gua::vec3(mesh.mTangents[i]);

      pBitangent = to_gua::vec3(mesh.mBitangents[i]);
    }

    positions.push_back(pPos);
    normals.push_back(pNormal);
    bitangents.push_back(pBitangent);
    tangents.push_back(pTangent);
    texCoords.push_back(pTexCoord);
  }

  //get weights and write them to vectors
  auto temp_weights = get_weights(mesh, root);
  for(auto const& w : temp_weights){
    for(uint i(0); i < w.weights.size(); ++i){
      bone_ids.push_back(w.IDs[i]);
      bone_weights.push_back(w.weights[i]);
    }
    bone_counts.push_back(w.weights.size());
  }

  // Populate the index buffer
  for(uint i = 0 ; i < mesh.mNumFaces ; i++) {
    const aiFace& face = mesh.mFaces[i];
    // triangulate face if necessary
    for(unsigned j = 2; j < face.mNumIndices; ++j) {
      indices.push_back(face.mIndices[0]);
      indices.push_back(face.mIndices[j - 1]);
      indices.push_back(face.mIndices[j]);
    }
  }
}

std::vector<bone_influences> SkinnedMesh::get_weights(aiMesh const& mesh, Bone const& root) {
  std::map<std::string, int> bone_mapping_; // maps a bone name to its index
  root.collect_indices(bone_mapping_);
  
  std::vector<bone_influences> temp_weights{mesh.mNumVertices};

  for (uint i = 0 ; i < mesh.mNumBones ; i++) {
    std::string bone_name(mesh.mBones[i]->mName.data);      
    uint BoneIndex = bone_mapping_.at(bone_name);        
    
    for (uint j = 0 ; j < mesh.mBones[i]->mNumWeights ; j++) {
      uint VertexID = mesh.mBones[i]->mWeights[j].mVertexId;
      float Weight  = mesh.mBones[i]->mWeights[j].mWeight;                   
      temp_weights[VertexID].add_bone(BoneIndex, Weight);
    }
  }

  return temp_weights;
}

std::vector<bone_influences> SkinnedMesh::get_weights(FbxMesh const& mesh, Bone const& root) {
  std::map<std::string, int> bone_mapping_; // maps a bone name to its index
  root.collect_indices(bone_mapping_);

  //check for skinning
  FbxSkin* skin = NULL;
  for(unsigned i = 0; i < mesh.GetDeformerCount(); ++i) {
    FbxDeformer* defPtr ={mesh.GetDeformer(i)};
    if(defPtr->GetDeformerType() == FbxDeformer::eSkin) {
      skin = static_cast<FbxSkin*>(defPtr);
      break;
    }
  }

  if(!skin) {
    Logger::LOG_WARNING << "Mesh does not contain skin deformer, ignoring weights" << std::endl;
    return std::vector<bone_influences>{};
  }
  //set up temporary weights, for control points not actual vertices
  std::vector<bone_influences> temp_weights{unsigned(mesh.GetControlPointsCount())};
  //one cluster corresponds to one bone
  for(unsigned i = 0; i < skin->GetClusterCount(); ++i) {
    FbxCluster* cluster = skin->GetCluster(i);
    FbxNode* Bone = cluster->GetLink();

    if(!Bone) {
      Logger::LOG_ERROR << "associated Bone does not exist!" << std::endl;
      assert(false);      
    }

    std::string bone_name(Bone->GetName());
    uint bone_index;
    if(bone_mapping_.find(bone_name) != bone_mapping_.end()) {
      bone_index = bone_mapping_.at(bone_name);
    }      
    else {
      Logger::LOG_ERROR << "Bone with name '" << bone_name << "' does not exist!, ignoring weights" << std::endl;
      return std::vector<bone_influences>{};          
    }

    FbxAMatrix world_transform;
    //reference pose of mes
    cluster->GetTransformMatrix(world_transform);

    int* indices = cluster->GetControlPointIndices();
    double* weights = cluster->GetControlPointWeights();
    for(unsigned i = 0; i < cluster->GetControlPointIndicesCount(); ++i) {
      //update mapping info of current control point
      temp_weights[indices[i]].add_bone(bone_index, weights[i]);
    }
  }

  return temp_weights;
}

void SkinnedMesh::copy_to_buffer(SkinnedVertex* vertex_buffer, uint resource_offset)  const {
  uint bone_offset{resource_offset};
  for (unsigned v(0); v < num_vertices; ++v) {

    vertex_buffer[v].pos = positions[v];

    vertex_buffer[v].tex = texCoords[v];

    vertex_buffer[v].normal = normals[v];

    vertex_buffer[v].tangent = tangents[v];

    vertex_buffer[v].bitangent = bitangents[v];

    vertex_buffer[v].bone_id_offset = bone_offset;

    vertex_buffer[v].nr_of_bones = bone_counts[v];
    
    bone_offset += bone_counts[v];
  }
}

scm::gl::vertex_format SkinnedMesh::get_vertex_format() const {
  return scm::gl::vertex_format(
    0, 0, scm::gl::TYPE_VEC3F, sizeof(SkinnedVertex))(
    0, 1, scm::gl::TYPE_VEC2F, sizeof(SkinnedVertex))(
    0, 2, scm::gl::TYPE_VEC3F, sizeof(SkinnedVertex))(
    0, 3, scm::gl::TYPE_VEC3F, sizeof(SkinnedVertex))(
    0, 4, scm::gl::TYPE_VEC3F, sizeof(SkinnedVertex))(
    0, 5, scm::gl::TYPE_UINT, sizeof(SkinnedVertex))(
    0, 6, scm::gl::TYPE_UINT, sizeof(SkinnedVertex));
}

std::vector<uint> const& 
SkinnedMesh::get_bone_ids()const{
  return bone_ids;
}

std::vector<float> const& 
SkinnedMesh::get_bone_weights()const{
  return bone_weights;
}

} // namespace gua