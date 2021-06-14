#pragma once

#include "linalg.h"
using namespace linalg::aliases;

#include "framebuffer.h"
#include "shader_program.h"
#include "filter.h"

#include <vector>
#include <functional>

class Renderer;
struct PassParameters;

class FilterChain {
public:
	void render(Renderer* renderer, PassParameters params, const std::function<void(uint32_t)>& iteration = nullptr);

	void create(uint32_t w, uint32_t h);

	void addFilter(Filter* filter) { m_filters.push_back(std::unique_ptr<Filter>(filter)); }
	Texture result() { return m_pingPongBuffer.colorAttachments()[1 - m_slot]; }
	Framebuffer& pingPongBuffer() { return m_pingPongBuffer; }
	uint32_t slot() const { return 1 - m_slot; }

	Filter* get(uint32_t index) { return m_filters[0].get(); }

private:
	Framebuffer m_pingPongBuffer;
	std::vector<std::unique_ptr<Filter>> m_filters{};
	uint32_t m_slot{ 1 };
};

