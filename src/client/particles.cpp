/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "particles.h"
#include <cmath>
#include "client.h"
#include "collision.h"
#include "client/content_cao.h"
#include "client/clientevent.h"
#include "client/renderingengine.h"
#include "util/numeric.h"
#include "light.h"
#include "environment.h"
#include "clientmap.h"
#include "mapnode.h"
#include "nodedef.h"
#include "client.h"
#include "settings.h"

/*
	Utility
*/

static f32 random_f32(f32 min, f32 max)
{
	return rand() / (float)RAND_MAX * (max - min) + min;
}

static v3f random_v3f(v3f min, v3f max)
{
	return v3f(
		random_f32(min.X, max.X),
		random_f32(min.Y, max.Y),
		random_f32(min.Z, max.Z));
}

/*
	Particle
*/

Particle::Particle(
	IGameDef *gamedef,
	ClientEnvironment *env,
	const ParticleParameters &p,
	video::ITexture *texture,
	v2f texpos,
	v2f texsize,
	video::SColor color
)
{
	// Misc
	m_gamedef = gamedef;
	m_env = env;

	// Texture
	m_material.setFlag(video::EMF_LIGHTING, false);
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, false);
	m_material.setFlag(video::EMF_BILINEAR_FILTER, false);
	m_material.setFlag(video::EMF_FOG_ENABLE, true);
	m_material.setFlag(video::EMF_ZWRITE_ENABLE, false);
	m_material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	m_material.setTexture(0, texture);
	m_texpos = texpos;
	m_texsize = texsize;
	m_animation = p.animation;

	// Color
	m_base_color = color;
	m_color = color;

	// Particle related
	m_pos = p.pos;
	m_velocity = p.vel;
	m_acceleration = p.acc;
	m_expiration = p.expirationtime;
	m_size = p.size;
	m_collisiondetection = p.collisiondetection;
	m_collision_removal = p.collision_removal;
	m_object_collision = p.object_collision;
	m_vertical = p.vertical;
	m_glow = p.glow;

	// Irrlicht stuff
	const float c = p.size / 2;
	m_collisionbox = aabb3f(-c, -c, -c, c, c, c);

	// Init lighting
	updateLight();

	// Init model
	updateVertices(ParticleFrameView(env));
}

ParticleFrameView::ParticleFrameView(ClientEnvironment *env)
{
	LocalPlayer *player = env->getLocalPlayer();
	player_pos = player->getPosition() / BS;
	camera_offset = env->getCameraOffset();

	const f32 pitch = player->getPitch() * core::DEGTORAD;
	const f32 yaw = player->getYaw() * core::DEGTORAD;
	pitch_sin = std::sin(pitch);
	pitch_cos = std::cos(pitch);
	yaw_sin = std::sin(yaw);
	yaw_cos = std::cos(yaw);

	const MapDrawControl &control = env->getClientMap().getControl();
	range_sq = control.range_all ? -1.0f
		: control.wanted_range * control.wanted_range;

	// Particles are stepped before the camera is moved, so this still describes
	// the previous frame
	has_cone = false;
	const scene::ICameraSceneNode *camera =
		RenderingEngine::get_scene_manager()->getActiveCamera();
	if (camera) {
		view_pos = camera->getAbsolutePosition();
		view_dir = camera->getTarget() - view_pos;
		if (view_dir.getLengthSQ() > 0.0f) {
			view_dir.normalize();
			const f32 aspect = camera->getAspectRatio();
			const f32 half = std::atan(std::tan(camera->getFOV() * 0.5f) *
				std::sqrt(1.0f + aspect * aspect));
			const f32 cos_half = std::cos(std::min(
				half + 20.0f * core::DEGTORAD, core::PI * 0.5f));
			view_cos_sq = cos_half * cos_half;
			has_cone = true;
		}
	}
}

Particle::~Particle()
{
	if (m_buffer)
		m_buffer->release(m_index);
}

bool Particle::attachToBuffer(ParticleBuffer *buffer)
{
	auto index = buffer->allocate();
	if (!index.has_value())
		return false;

	m_index = index.value();
	m_buffer = buffer;
	return true;
}

void Particle::step(float dtime, const ParticleFrameView &view)
{
	m_time += dtime;
	if (m_collisiondetection) {
		aabb3f box = m_collisionbox;
		v3f p_pos = m_pos * BS;
		v3f p_velocity = m_velocity * BS;
		collisionMoveResult r = collisionMoveSimple(m_env, m_gamedef, BS * 0.5f,
			box, 0.0f, dtime, &p_pos, &p_velocity, m_acceleration * BS, nullptr,
			m_object_collision);
		if (m_collision_removal && r.collides) {
			// force expiration of the particle
			m_expiration = -1.0;
		} else {
			m_pos = p_pos / BS;
			m_velocity = p_velocity / BS;
		}
	} else {
		m_velocity += m_acceleration * dtime;
		m_pos += m_velocity * dtime;
	}
	if (m_animation.type != TAT_NONE) {
		m_animation_time += dtime;
		int frame_length_i, frame_count;
		m_animation.determineParams(
				m_material.getTexture(0)->getSize(),
				&frame_count, &frame_length_i, NULL);
		float frame_length = frame_length_i / 1000.0;
		while (m_animation_time > frame_length) {
			m_animation_frame++;
			m_animation_time -= frame_length;
		}
	}

	// Out of range or out of sight there is nothing to light or to shape
	bool drawn = view.range_sq < 0.0f ||
		m_pos.getDistanceFromSQ(view.player_pos) <= view.range_sq;
	if (drawn && view.has_cone) {
		const v3f offset = m_pos * BS
			- intToFloat(view.camera_offset, BS) - view.view_pos;
		const f32 dist_sq = offset.getLengthSQ();
		const f32 along = offset.dotProduct(view.view_dir);
		// Same as along > view_cos * sqrt(dist_sq), without the root.
		// Right on top of the camera the direction says nothing useful.
		drawn = dist_sq < BS * BS ||
			(along > 0.0f && along * along > view.view_cos_sq * dist_sq);
	}
	if (drawn != m_drawn) {
		if (m_buffer)
			m_buffer->setQuadDrawn(m_index, drawn);
		m_drawn = drawn;
	}
	if (!drawn)
		return;

	// Update lighting
	updateLight();

	// Update model
	updateVertices(view);
}

void Particle::updateLight()
{
	u8 light = 0;
	bool pos_ok;

	v3s16 p = v3s16(
		floor(m_pos.X+0.5),
		floor(m_pos.Y+0.5),
		floor(m_pos.Z+0.5)
	);
	MapNode n = m_env->getClientMap().getNode(p, &pos_ok);
	if (pos_ok)
		light = n.getLightBlend(m_env->getDayNightRatio(), m_gamedef->ndef());
	else
		light = blend_light(m_env->getDayNightRatio(), LIGHT_SUN, 0);

	u8 m_light = decode_light(light + m_glow);
	m_color.set(255,
		m_light * m_base_color.getRed() / 255,
		m_light * m_base_color.getGreen() / 255,
		m_light * m_base_color.getBlue() / 255);
}

void Particle::updateVertices(const ParticleFrameView &view)
{
	if (!m_buffer)
		return;

	video::S3DVertex *m_vertices = m_buffer->getVertices(m_index);
	f32 tx0, tx1, ty0, ty1;

	if (m_animation.type != TAT_NONE) {
		const v2u32 texsize = m_material.getTexture(0)->getSize();
		v2f texcoord, framesize_f;
		v2u32 framesize;
		texcoord = m_animation.getTextureCoords(texsize, m_animation_frame);
		m_animation.determineParams(texsize, NULL, NULL, &framesize);
		framesize_f = v2f(framesize.X / (float) texsize.X, framesize.Y / (float) texsize.Y);

		tx0 = m_texpos.X + texcoord.X;
		tx1 = m_texpos.X + texcoord.X + framesize_f.X * m_texsize.X;
		ty0 = m_texpos.Y + texcoord.Y;
		ty1 = m_texpos.Y + texcoord.Y + framesize_f.Y * m_texsize.Y;
	} else {
		tx0 = m_texpos.X;
		tx1 = m_texpos.X + m_texsize.X;
		ty0 = m_texpos.Y;
		ty1 = m_texpos.Y + m_texsize.Y;
	}

	m_vertices[0] = video::S3DVertex(-m_size / 2, -m_size / 2,
		0, 0, 0, 0, m_color, tx0, ty1);
	m_vertices[1] = video::S3DVertex(m_size / 2, -m_size / 2,
		0, 0, 0, 0, m_color, tx1, ty1);
	m_vertices[2] = video::S3DVertex(m_size / 2, m_size / 2,
		0, 0, 0, 0, m_color, tx1, ty0);
	m_vertices[3] = video::S3DVertex(-m_size / 2, m_size / 2,
		0, 0, 0, 0, m_color, tx0, ty0);

	// All four corners turn by the same angles, so the trigonometry is done
	// once here rather than inside every rotate call
	f32 yz_sin = 0.0f, yz_cos = 1.0f, xz_sin, xz_cos;
	if (m_vertical) {
		const f32 yaw = std::atan2(view.player_pos.Z - m_pos.Z,
			view.player_pos.X - m_pos.X) + core::PI / 2;
		xz_sin = std::sin(yaw);
		xz_cos = std::cos(yaw);
	} else {
		yz_sin = view.pitch_sin;
		yz_cos = view.pitch_cos;
		xz_sin = view.yaw_sin;
		xz_cos = view.yaw_cos;
	}

	const v3f origin = m_pos * BS - intToFloat(view.camera_offset, BS);
	for (u16 i = 0; i < 4; i++) {
		v3f &pos = m_vertices[i].Pos;
		const v3f p = pos;
		const f32 z = p.Y * yz_sin + p.Z * yz_cos;
		pos.set(p.X * xz_cos - z * xz_sin,
			p.Y * yz_cos - p.Z * yz_sin,
			p.X * xz_sin + z * xz_cos);
		pos += origin;
	}
}

/*
	ParticleSpawner
*/

ParticleSpawner::ParticleSpawner(
	IGameDef *gamedef,
	LocalPlayer *player,
	const ParticleSpawnerParameters &p,
	u16 attached_id,
	video::ITexture *texture,
	ParticleManager *p_manager
):
	m_particlemanager(p_manager), p(p)
{
	m_gamedef = gamedef;
	m_player = player;
	m_attached_id = attached_id;
	m_texture = texture;
	m_time = 0;

	m_spawntimes.reserve(p.amount + 1);
	for (u16 i = 0; i <= p.amount; i++) {
		float spawntime = rand() / (float)RAND_MAX * p.time;
		m_spawntimes.push_back(spawntime);
	}
}

void ParticleSpawner::spawnParticle(ClientEnvironment *env, float radius,
	const core::matrix4 *attached_absolute_pos_rot_matrix)
{
	v3f ppos = m_player->getPosition() / BS;
	v3f pos = random_v3f(p.minpos, p.maxpos);

	// Need to apply this first or the following check
	// will be wrong for attached spawners
	if (attached_absolute_pos_rot_matrix) {
		pos *= BS;
		attached_absolute_pos_rot_matrix->transformVect(pos);
		pos /= BS;
		v3s16 camera_offset = m_particlemanager->m_env->getCameraOffset();
		pos.X += camera_offset.X;
		pos.Y += camera_offset.Y;
		pos.Z += camera_offset.Z;
	}

	if (pos.getDistanceFrom(ppos) > radius)
		return;

	// Parameters for the single particle we're about to spawn
	ParticleParameters pp;
	pp.pos = pos;

	pp.vel = random_v3f(p.minvel, p.maxvel);
	pp.acc = random_v3f(p.minacc, p.maxacc);

	if (attached_absolute_pos_rot_matrix) {
		// Apply attachment rotation
		attached_absolute_pos_rot_matrix->rotateVect(pp.vel);
		attached_absolute_pos_rot_matrix->rotateVect(pp.acc);
	}

	pp.expirationtime = random_f32(p.minexptime, p.maxexptime);
	p.copyCommon(pp);

	video::ITexture *texture;
	v2f texpos, texsize;
	video::SColor color(0xFFFFFFFF);

	if (p.node.getContent() != CONTENT_IGNORE) {
		const ContentFeatures &f =
			m_particlemanager->m_env->getGameDef()->ndef()->get(p.node);
		if (!ParticleManager::getNodeParticleParams(p.node, f, pp, &texture,
				texpos, texsize, &color, p.node_tile))
			return;
	} else {
		texture = m_texture;
		texpos = v2f(0.0f, 0.0f);
		texsize = v2f(1.0f, 1.0f);
	}

	// Allow keeping default random size
	if (p.maxsize > 0.0f)
		pp.size = random_f32(p.minsize, p.maxsize);

	m_particlemanager->addParticle(new Particle(
		m_gamedef,
		env,
		pp,
		texture,
		texpos,
		texsize,
		color
	));
}

void ParticleSpawner::step(float dtime, ClientEnvironment *env)
{
	m_time += dtime;

	static thread_local const float radius =
			g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE;

	bool unloaded = false;
	const core::matrix4 *attached_absolute_pos_rot_matrix = nullptr;
	if (m_attached_id) {
		if (GenericCAO *attached = dynamic_cast<GenericCAO *>(env->getActiveObject(m_attached_id))) {
			attached_absolute_pos_rot_matrix = attached->getAbsolutePosRotMatrix();
		} else {
			unloaded = true;
		}
	}

	if (p.time != 0) {
		// Spawner exists for a predefined timespan
		for (auto i = m_spawntimes.begin(); i != m_spawntimes.end(); ) {
			if ((*i) <= m_time && p.amount > 0) {
				--p.amount;

				// Pretend to, but don't actually spawn a particle if it is
				// attached to an unloaded object or distant from player.
				if (!unloaded)
					spawnParticle(env, radius, attached_absolute_pos_rot_matrix);

				i = m_spawntimes.erase(i);
			} else {
				++i;
			}
		}
	} else {
		// Spawner exists for an infinity timespan, spawn on a per-second base

		// Skip this step if attached to an unloaded object
		if (unloaded)
			return;

		for (int i = 0; i <= p.amount; i++) {
			if (rand() / (float)RAND_MAX < dtime)
				spawnParticle(env, radius, attached_absolute_pos_rot_matrix);
		}
	}
}

/*
	ParticleManager
*/

/*
	ParticleBuffer
*/

static constexpr u16 quad_indices[] = { 0, 1, 2, 2, 3, 0 };

ParticleBuffer::ParticleBuffer(ClientEnvironment *env,
	const video::SMaterial &material
):
	scene::ISceneNode(RenderingEngine::get_scene_manager()->getRootSceneNode(),
		RenderingEngine::get_scene_manager()),
	m_mesh_buffer(make_irr<scene::SMeshBuffer>())
{
	m_mesh_buffer->getMaterial() = material;
}

std::optional<u16> ParticleBuffer::allocate()
{
	m_usage_timer = 0;

	if (!m_free_list.empty()) {
		const u16 index = m_free_list.back();
		m_free_list.pop_back();
		auto *vertices = static_cast<video::S3DVertex *>(
			m_mesh_buffer->getVertices());
		u16 *indices = m_mesh_buffer->getIndices();
		// reset vertices, because they are only written in Particle::step()
		for (u16 i = 0; i < 4; i++)
			vertices[4 * index + i] = video::S3DVertex();
		for (u16 i = 0; i < 6; i++)
			indices[6 * index + i] = 4 * index + quad_indices[i];
		return index;
	}

	if (m_count >= MAX_PARTICLES_PER_BUFFER)
		return std::nullopt;

	// The buffer never shrinks, ParticleManager drops it once it falls idle
	video::S3DVertex vertices[4] = {};
	m_mesh_buffer->append(vertices, 4, quad_indices, 6);
	return m_count++;
}

void ParticleBuffer::release(u16 index)
{
	assert(index < m_count);
	u16 *indices = m_mesh_buffer->getIndices();
	for (u16 i = 0; i < 6; i++)
		indices[6 * index + i] = 0;
	m_free_list.push_back(index);
}

void ParticleBuffer::setQuadDrawn(u16 index, bool drawn)
{
	assert(index < m_count);
	u16 *indices = m_mesh_buffer->getIndices();
	for (u16 i = 0; i < 6; i++)
		indices[6 * index + i] = drawn ? 4 * index + quad_indices[i] : 0;
	m_bounding_box_dirty = true;
}

video::S3DVertex *ParticleBuffer::getVertices(u16 index)
{
	if (index >= m_count)
		return nullptr;
	m_bounding_box_dirty = true;
	return &static_cast<video::S3DVertex *>(m_mesh_buffer->getVertices())[4 * index];
}

void ParticleBuffer::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this,
			scene::ESNRP_TRANSPARENT_EFFECT);

	scene::ISceneNode::OnRegisterSceneNode();
}

const aabb3f &ParticleBuffer::getBoundingBox() const
{
	if (!m_bounding_box_dirty)
		return m_mesh_buffer->BoundingBox;

	aabb3f box;
	for (u16 i = 0; i < m_count; i++) {
		// a zeroed index marks a slot nobody is using
		static_assert(quad_indices[1] != 0);
		if (m_mesh_buffer->getIndices()[6 * i + 1] == 0)
			continue;

		for (u16 j = 0; j < 4; j++)
			box.addInternalPoint(m_mesh_buffer->getPosition(i * 4 + j));
	}

	m_mesh_buffer->BoundingBox = box;
	m_bounding_box_dirty = false;
	return m_mesh_buffer->BoundingBox;
}

void ParticleBuffer::render()
{
	if (isEmpty())
		return;

	video::IVideoDriver *driver = SceneManager->getVideoDriver();
	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	driver->setMaterial(m_mesh_buffer->getMaterial());
	driver->drawMeshBuffer(m_mesh_buffer.get());
}

/*
	ParticleManager
*/

ParticleManager::ParticleManager(ClientEnvironment *env) :
	m_env(env)
{}

ParticleManager::~ParticleManager()
{
	clearAll();
}

void ParticleManager::step(float dtime)
{
	stepParticles (dtime);
	stepSpawners (dtime);
	stepBuffers (dtime);
}

void ParticleManager::stepBuffers(float dtime)
{
	constexpr float INTERVAL = 0.5f;
	if (!m_buffer_gc.step(dtime, INTERVAL))
		return;

	for (size_t i = 0; i < m_particle_buffers.size();) {
		auto &buffer = m_particle_buffers[i];
		buffer->m_usage_timer += INTERVAL;
		if (buffer->isEmpty() && buffer->m_usage_timer > 5.0f) {
			buffer->remove();
			buffer = std::move(m_particle_buffers.back());
			m_particle_buffers.pop_back();
		} else {
			i++;
		}
	}

}

void ParticleManager::stepSpawners(float dtime)
{
	MutexAutoLock lock(m_spawner_list_lock);
	for (auto i = m_particle_spawners.begin(); i != m_particle_spawners.end();) {
		if (i->second->get_expired()) {
			delete i->second;
			m_particle_spawners.erase(i++);
		} else {
			i->second->step(dtime, m_env);
			++i;
		}
	}
}

void ParticleManager::stepParticles(float dtime)
{
	MutexAutoLock lock(m_particle_list_lock);
	const ParticleFrameView view(m_env);
	for (auto i = m_particles.begin(); i != m_particles.end();) {
		if ((*i)->get_expired()) {
			delete *i;
			i = m_particles.erase(i);
		} else {
			(*i)->step(dtime, view);
			++i;
		}
	}
}

void ParticleManager::clearAll()
{
	MutexAutoLock lock(m_spawner_list_lock);
	MutexAutoLock lock2(m_particle_list_lock);
	for (auto i = m_particle_spawners.begin(); i != m_particle_spawners.end();) {
		delete i->second;
		m_particle_spawners.erase(i++);
	}

	for(auto i = m_particles.begin(); i != m_particles.end();)
	{
		delete *i;
		i = m_particles.erase(i);
	}
}

void ParticleManager::handleParticleEvent(ClientEvent *event, Client *client,
	LocalPlayer *player)
{
	switch (event->type) {
		case CE_DELETE_PARTICLESPAWNER: {
			deleteParticleSpawner(event->delete_particlespawner.id);
			// no allocated memory in delete event
			break;
		}
		case CE_ADD_PARTICLESPAWNER: {
			deleteParticleSpawner(event->add_particlespawner.id);

			const ParticleSpawnerParameters &p = *event->add_particlespawner.p;

			video::ITexture *texture =
				client->tsrc()->getTextureForMesh(p.texture);

			auto toadd = new ParticleSpawner(client, player,
					p,
					event->add_particlespawner.attached_id,
					texture,
					this);

			addParticleSpawner(event->add_particlespawner.id, toadd);

			delete event->add_particlespawner.p;
			break;
		}
		case CE_SPAWN_PARTICLE: {
			ParticleParameters &p = *event->spawn_particle;

			video::ITexture *texture;
			v2f texpos, texsize;
			video::SColor color(0xFFFFFFFF);

			f32 oldsize = p.size;

			if (p.node.getContent() != CONTENT_IGNORE) {
				const ContentFeatures &f = m_env->getGameDef()->ndef()->get(p.node);
				if (!getNodeParticleParams(p.node, f, p, &texture, texpos,
						texsize, &color, p.node_tile))
					texture = nullptr;
			} else {
				texture = client->tsrc()->getTextureForMesh(p.texture);
				texpos = v2f(0.0f, 0.0f);
				texsize = v2f(1.0f, 1.0f);
			}

			// Allow keeping default random size
			if (oldsize > 0.0f)
				p.size = oldsize;

			if (texture) {
				Particle *toadd = new Particle(client, m_env,
						p, texture, texpos, texsize, color);

				addParticle(toadd);
			}

			delete event->spawn_particle;
			break;
		}
		default: break;
	}
}

bool ParticleManager::getNodeParticleParams(const MapNode &n,
	const ContentFeatures &f, ParticleParameters &p, video::ITexture **texture,
	v2f &texpos, v2f &texsize, video::SColor *color, u8 tilenum)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return false;

	// Texture
	u8 texid;
	if (tilenum > 0 && tilenum <= 6)
		texid = tilenum - 1;
	else
		texid = rand() % 6;
	const TileLayer &tile = f.tiles[texid].layers[0];
	p.animation.type = TAT_NONE;

	// Only use first frame of animated texture
	if (tile.material_flags & MATERIAL_FLAG_ANIMATION)
		*texture = (*tile.frames)[0].texture;
	else
		*texture = tile.texture;

	float size = (rand() % 8) / 64.0f;
	p.size = BS * size;
	if (tile.scale)
		size /= tile.scale;
	texsize = v2f(size * 2.0f, size * 2.0f);
	texpos.X = (rand() % 64) / 64.0f - texsize.X;
	texpos.Y = (rand() % 64) / 64.0f - texsize.Y;

	if (tile.has_color)
		*color = tile.color;
	else
		n.getColor(f, color);

	return true;
}

// The final burst of particles when a node is finally dug, *not* particles
// spawned during the digging of a node.

void ParticleManager::addDiggingParticles(IGameDef *gamedef,
	LocalPlayer *player, v3s16 pos, const MapNode &n, const ContentFeatures &f)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return;

	for (u16 j = 0; j < 16; j++) {
		addNodeParticle(gamedef, player, pos, n, f);
	}
}

// During the digging of a node particles are spawned individually by this
// function, called from Game::handleDigging() in game.cpp.

void ParticleManager::addNodeParticle(IGameDef *gamedef,
	LocalPlayer *player, v3s16 pos, const MapNode &n, const ContentFeatures &f)
{
	ParticleParameters p;
	video::ITexture *texture;
	v2f texpos, texsize;
	video::SColor color;

	if (!getNodeParticleParams(n, f, p, &texture, texpos, texsize, &color))
		return;

	p.expirationtime = (rand() % 100) / 100.0f;

	// Physics
	p.vel = v3f(
		(rand() % 150) / 50.0f - 1.5f,
		(rand() % 150) / 50.0f,
		(rand() % 150) / 50.0f - 1.5f
	);
	p.acc = v3f(
		0.0f,
		-player->movement_gravity * player->physics_override_gravity / BS,
		0.0f
	);
	p.pos = v3f(
		(f32)pos.X + (rand() % 100) / 200.0f - 0.25f,
		(f32)pos.Y + (rand() % 100) / 200.0f - 0.25f,
		(f32)pos.Z + (rand() % 100) / 200.0f - 0.25f
	);

	Particle *toadd = new Particle(
		gamedef,
		m_env,
		p,
		texture,
		texpos,
		texsize,
		color);

	addParticle(toadd);
}

bool ParticleManager::addParticle(Particle *toadd)
{
	MutexAutoLock lock(m_particle_list_lock);

	const video::SMaterial &material = toadd->getParticleMaterial();
	ParticleBuffer *found = nullptr;

	// shortcut for the common case of many particles of one kind in a row
	if (!m_particles.empty()) {
		Particle *last = m_particles.back();
		if (last->getBuffer() && last->getBuffer()->getMaterial(0) == material)
			found = last->getBuffer();
	}
	if (!found) {
		for (auto &buffer : m_particle_buffers) {
			if (buffer->getMaterial(0) == material) {
				found = buffer.get();
				break;
			}
		}
	}
	if (!found) {
		auto created = make_irr<ParticleBuffer>(m_env, material);
		found = created.get();
		m_particle_buffers.push_back(std::move(created));
	}

	if (!toadd->attachToBuffer(found)) {
		delete toadd;
		return false;
	}

	m_particles.push_back(toadd);
	return true;
}


void ParticleManager::addParticleSpawner(u64 id, ParticleSpawner *toadd)
{
	MutexAutoLock lock(m_spawner_list_lock);
	m_particle_spawners[id] = toadd;
}

void ParticleManager::deleteParticleSpawner(u64 id)
{
	MutexAutoLock lock(m_spawner_list_lock);
	auto it = m_particle_spawners.find(id);
	if (it != m_particle_spawners.end()) {
		delete it->second;
		m_particle_spawners.erase(it);
	}
}
