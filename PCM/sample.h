#ifndef _EXAMPLE_H
#define _EXAMPLE_H
#include "CustomGL/glew.h"
#include "QGLViewer/frame.h"
#include "selectable_item.h"
#include "pool_allocator.h"
#include "box.h"
#include "..\ICP\ICP.h"
#include "Eigen\Dense"
#include "basic_types.h"
#include "rendering/render_types.h"
#include "file_io.h"
#include <QMutex>
#include <set>
#include <vertex.h>

class TriangleType;
class LinkNode;
class KDTree;
class HitResult;
namespace MyOpengl
{
	class MeshOpengl;
}
namespace pcm
{
	class Scene;
}


class Sample:public SelectableItem
{
public:

	Sample();
	~Sample();
	void clear();

	inline Vertex& operator[]( IndexType i ) 
	{ 
		return *vertices_[i]; 	
	}
	inline TriangleType& getTriangle(IndexType i)
	{ 
		return *triangle_array[i]; 
	}
	Vertex* add_vertex( const pcm::PointType& pos,const pcm::NormalType& n,
		const pcm::ColorType& c);
	TriangleType* add_triangle(const TriangleType& tt);
	pcm::Scene*  load_scene(std::string path);

	void delete_vertex_group( const std::vector<IndexType>& idx_grp );
	void set_vertex_label(const std::vector<IndexType>& idx_grp ,IndexType label);

	virtual bool is_visible() const { return visible_; }
	virtual void set_visble(const bool v);
	virtual bool is_selected() const { return selected_; }
	virtual void set_selected( const bool s ){ selected_ = s; }

	void draw(ColorMode::ObjectColorMode&, const Vec3& bias = Vec3(0.,0.,0.));
	void draw(ColorMode::VertexColorMode&,const Vec3& bias = Vec3(0.,0.,0.));
	void draw(ColorMode::LabelColorMode&,const Vec3& bias = Vec3(0.,0.,0.));
	void draw(ColorMode::WrapBoxColorMode&, const Vec3& bias = Vec3(0.,0.,0.)); // add by huayun
	void draw(ColorMode::EdgePointColorMode&, const Vec3& bias = Vec3(0.,0.,0.)); // add by huayun
	void draw(ColorMode::SphereMode&,const Vec3& bias  = Vec3(0.,0.,0.));
	void draw( RenderMode::WhichColorMode& wcm ,RenderMode::RenderType& r ,const Vec3& bias  = Vec3(0.,0.,0.));
	void drawNormal(const Vec3& bias  = Vec3(0.,0.,0.) );
	//vector< map<IndexType,Vertex*> >				lb_wrapbox_;
	//vector< set<LinkNode> >				wrap_box_link_;
	//void addWrapBox( std::map<IndexType,Vertex*>  _l){ lb_wrapbox_.push_back(_l);}
	//void addWrapBoxLink( std::set<LinkNode> _l){ wrap_box_link_ .push_back(_l);}
	//void getWrapBox(IndexType _depth ,  std::map<IndexType,Vertex*>& out_)
	//{
	//	if(  (_depth > -1) && ((_depth < lb_wrapbox_.size())&& (lb_wrapbox_.size()>0)) ){
	//		out_ = lb_wrapbox_[_depth];
	//	}
	//}
	//void getWrapBoxLink( IndexType _depth ,  std::set<LinkNode>& out_){
	//	if(  (_depth > -1) && ((_depth < wrap_box_link_.size())&& (wrap_box_link_.size()>0)) ){
	//		out_ = wrap_box_link_[_depth];
	//	}
	//}
	
	void draw_with_name();
	void caculateNorm(pcm::NormalType& baseline = NULL_NORMAL);
	void caculateTangent();
	size_t num_vertices() const { return vertices_.size(); }
	size_t num_triangles() const { return triangle_array.size(); }

	typedef std::vector<Vertex*>::iterator	vtx_iterator;

	inline vtx_iterator begin(){ return vertices_.begin(); }
	inline vtx_iterator end(){ return vertices_.end(); }

	//Every time vertex change, the kdtree should rebuild
	void	build_kdtree();

	IndexType closest_vtx( const pcm::PointType& query_point );
	bool		neighbours(const IndexType query_point_idx, const IndexType num_closet, IndexType* out_indices);
	/* 
		Get matrix for transforming world-sample space to 
		view-sample space , making sure all samples can be saw
		no matter what original coordinates it is
	*/
	inline Matrix44 matrix_to_scene_coord(  );
	Matrix44 inverse_matrix_to_scene_coord();
	/* Green channel to get all vertex position information */
	inline  Matrix3X&	vertices_matrix()
	{	
		if (kd_tree_should_rebuild_)
		{
			build_kdtree();
		}
		return vtx_matrix_; 
	}
	/*Update vertex position according vertex matrix*/
	void	update();

	inline void lock(){ mutex_.lock(); }
	inline void unlock(){ mutex_.unlock(); }

	bool Sample::neighbours(const IndexType query_point_idx, const IndexType num_closet,
		IndexType* out_indices,ScalarType* out_distances);


	const pcm::PointType box_center() const{ return box_.center(); }
	const ScalarType	box_diag() { return box_.diag(); }
	const pcm::PointType box_near_corner(){ return box_.low_corner(); }
	const pcm::PointType box_far_corner(){ return box_.high_corner(); }
	
	Box getBox(){return box_;}
	inline qglviewer::Frame& getFrame()
	{
		return m_frame;
	}
	inline qglviewer::Vec getGlobalPosition(int vtx_idx)
	{
		Vertex* vetex = vertices_[vtx_idx];
		qglviewer::Vec pos(vetex->x(), vetex->y(), vetex->z());
		return m_frame.inverseCoordinatesOf(pos); //convert  frame coordinate to world coordinate 
	}
	inline void setGlobalPosition(int vtx_idx , qglviewer::Vec _pos)
	{
		Vertex* vetex = vertices_[vtx_idx];
		qglviewer::Vec pos =  m_frame.coordinatesOf(_pos);  //convert world coordinate to frame coordinate
		vetex->set_position(pcm::PointType(pos.x, pos.y, pos.z));
	}
	inline qglviewer::Vec getLocalPosition(int vtx_idx)
	{
		Vertex* vetex = vertices_[vtx_idx];
		qglviewer::Vec pos(vetex->x(), vetex->y(), vetex->z());
		return pos; 
	}

	inline void setLocalPosition(int vtx_idx , qglviewer::Vec _pos)
	{
		Vertex* vetex = vertices_[vtx_idx];
		vetex->set_position(pcm::PointType(_pos.x, _pos.y, _pos.z));
	}


	bool load();
	bool unload();
	bool isLoaded(){ return isload_;}
	void setLoaded(bool _isloaded){ isload_ = _isloaded; }
	void smoothLabel();
// public:
// 	QMutex										mutex_;
public:
	IndexType smpId;  //added by huayun
	FileIO::FILE_TYPE file_type;
	std::string file_path;

	RenderMode::WeightColorMode color_mode;
	std::vector<pcm::ColorType> colors_;
	
private:
	qglviewer::Frame m_frame;
	bool isload_;
	std::vector<Vertex*>	vertices_;
	std::vector<TriangleType*>  triangle_array;
public:
	void update_openglMesh();
	void update_openglMeshColor();
	void setOpenglMeshUpdated(bool isUpdated)
	{
		isOpenglMeshUpdated = isUpdated;
	}
	void setOpenglMeshColorUpdated(bool isUpdated)
	{
		isOpenglMeshColorUpdated = isUpdated;
	}
	bool isScaledToUniform()
	{
		return isScaledToUniform_;
	}
	void setIsScaleToUniform(bool b )
	{
		isScaledToUniform_ = b;
	}
	std::vector<Vertex*>& getVerticesArray()
	{
		return vertices_;
	}
	std::vector<TriangleType*>& getTriangleArray()
	{
		return triangle_array;
	}
	void worldRaytoLocal(const Ray& world_ray, Ray& local_ray);
	void localRayToWorld(const Ray& local_ray, Ray&world_ray);
	bool castray(Ray& world_ray,HitResult& result);
	void clearKdTreeRayBuffer();
	void updateHitrayBuffer();
private:
	bool isScaledToUniform_;
	bool isOpenglMeshUpdated;
	bool isOpenglMeshColorUpdated;
	bool isUsingProgramablePipeLine;
	MyOpengl::MeshOpengl* opengl_mesh_;
	pcm::Scene*					scene_;



	PoolAllocator			allocator_;
	Box						box_;
	//added by huayun
	
	 
	Matrix3X									vtx_matrix_; // 3 * NumOfVtx
	//Attention: kdtree is just a adapter, it means it use the reference of its data source
	nanoflann::KDTreeAdaptor<Matrix3X, 3>*		kd_tree_;
	KDTree*										kd_tree_raycast_;
	bool										kd_tree_should_rebuild_;
	QMutex										mutex_;
public:
	std::vector< std::map<IndexType,Vertex*>>				lb_wrapbox_;

	std::vector< std::vector<LinkNode> >							wrap_box_link_;
	IndexType				clayerDepth_;
};

class LinkNode
{
public:
	IndexType	labelH_;
	IndexType	labelL_;	
	pcm::PointType	pointH_;
	pcm::PointType	pointL_;
	LinkNode( IndexType	_labelH ,IndexType	_labelL, pcm::PointType		_pointH, pcm::PointType	_pointL):
		labelH_(_labelH) ,labelL_(_labelL) ,pointH_(_pointH),pointL_(_pointL){}
	//Vertex*		vtxH_;
	//Vertex*		vtxL_;

	//LinkNode( IndexType	_labelH ,IndexType	_labelL, Vertex*		_vtexH, Vertex*		_vtexL):
	//	labelH_(_labelH) ,labelL_(_labelL) ,vtxH_(_vtexH),vtxL_(_vtexL)
	//{

	//}
	//~LinkNode(){
	//	if(vtxH_)delete	vtxH_;
	//	if(vtxL_)delete	vtxL_;

	//}
};

#endif