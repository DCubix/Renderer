#include "filter_chain.h"

#include "renderer.h"

void FilterChain::render(Renderer* renderer, PassParameters params, const std::function<void(uint32_t)>& iteration) {
	for (auto& filter : m_filters) {
		if (!filter->valid()) filter->create();
	}

	m_pingPongBuffer.bind();
	for (auto& filter : m_filters) {
		filter->bind();
		for (uint32_t i = 0; i < filter->repeatCount(); i++) {
			m_pingPongBuffer.setDrawBuffer(m_slot);
			glClear(GL_COLOR_BUFFER_BIT);

			if (iteration) iteration(i);
			filter->setUniforms();

			renderer->renderScreenQuad();

			m_slot = (1 - m_slot);
		}
	}
	m_pingPongBuffer.unbind(true);
}

void FilterChain::create(uint32_t w, uint32_t h) {
	if (!m_pingPongBuffer.valid()) {
		m_pingPongBuffer.create(w, h);
		m_pingPongBuffer.addColorAttachment(TextureFormat::RGBAf, TextureTarget::Texture2D);
		m_pingPongBuffer.addColorAttachment(TextureFormat::RGBAf, TextureTarget::Texture2D);
	}
}
