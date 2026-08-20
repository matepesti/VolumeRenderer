#include "SmoothPass.h"

GLuint SmoothPass::run(GLuint sourceTexture, int nx, int ny, int nz, float sigma) {
	if (m_texA == 0 || m_texB == 0 || m_nx != nx || m_ny != ny || m_nz != nz) {
		glDeleteTextures(1, &m_texA);
		glDeleteTextures(1, &m_texB);

		glGenTextures(1, &m_texA);
		glBindTexture(GL_TEXTURE_3D, m_texA);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, nx, ny, nz);

		glGenTextures(1, &m_texB);
		glBindTexture(GL_TEXTURE_3D, m_texB);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, nx, ny, nz);

		m_nx = nx;
		m_ny = ny;
		m_nz = nz;

	}

	glCopyImageSubData(sourceTexture, GL_TEXTURE_3D, 0, 0, 0, 0, m_texA, GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz);

	compShader.use();
	glBindImageTexture(0, m_texA, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(1, m_texB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
	compShader.setInt("uAxis", 0);
	compShader.setFloat("uSigma",sigma);
	glDispatchCompute((nx + 7) / 8, (ny + 7) / 8, (nz + 7) / 8);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindImageTexture(0, m_texB, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(1, m_texA, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
	compShader.setInt("uAxis", 1);
	glDispatchCompute((nx + 7) / 8, (ny + 7) / 8, (nz + 7) / 8);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindImageTexture(0, m_texA, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(1, m_texB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
	compShader.setInt("uAxis", 2);
	glDispatchCompute((nx + 7) / 8, (ny + 7) / 8, (nz + 7) / 8);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	return m_texB;
}

void SmoothPass::init(const std::string& compPath) {
	compShader.compileCompute(compPath);
}
