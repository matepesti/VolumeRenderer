#include "SmoothPass.h"

GLuint SmoothPass::run(GLuint sourceTexture,int nx,int ny,int nz,float sigma){
    if (sourceTexture == 0 || nx <= 0 || ny <= 0 || nz <= 0 || sigma <= 0.01f){
        return 0;
    }

    const bool dimensionsChanged = m_nx != nx || m_ny != ny || m_nz != nz;

    if (m_texA == 0 || m_texB == 0 || dimensionsChanged) {
        release();

        glGenTextures(1,&m_texA);
        glBindTexture(GL_TEXTURE_3D,m_texA);
        glTexStorage3D(GL_TEXTURE_3D,1,GL_R32F,nx,ny,nz);
        glGenTextures(1,&m_texB);
        glBindTexture(GL_TEXTURE_3D,m_texB);
        glTexStorage3D(GL_TEXTURE_3D,1,GL_R32F,nx,ny,nz);
        glBindTexture(GL_TEXTURE_3D,0);

        m_nx = nx;
        m_ny = ny;
        m_nz = nz;
    }

    // Copy the original volume into the first working texture.
    glCopyImageSubData(sourceTexture,GL_TEXTURE_3D,0,0, 0, 0,m_texA,GL_TEXTURE_3D,0,0, 0, 0,nx,ny,nz);

    // x direction
    compShader.use();

    glBindImageTexture(0,m_texA,0,GL_TRUE,0,GL_READ_ONLY,GL_R32F);
    glBindImageTexture(1,m_texB,0,GL_TRUE,0,GL_WRITE_ONLY,GL_R32F);

    compShader.setInt("uAxis",0);
    compShader.setFloat("uSigma",sigma);

    glDispatchCompute((nx + 7) / 8,(ny + 7) / 8,(nz + 7) / 8);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // y direction
    glBindImageTexture(0,m_texB,0,GL_TRUE,0,GL_READ_ONLY,GL_R32F);
    glBindImageTexture(1,m_texA,0,GL_TRUE,0,GL_WRITE_ONLY,GL_R32F);

    compShader.setInt("uAxis",1);

    glDispatchCompute((nx + 7) / 8,(ny + 7) / 8,(nz + 7) / 8);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // z direction

    glBindImageTexture(0,m_texA,0,GL_TRUE,0,GL_READ_ONLY,GL_R32F);
    glBindImageTexture(1,m_texB,0,GL_TRUE,0,GL_WRITE_ONLY,GL_R32F);

    compShader.setInt("uAxis",2);

    glDispatchCompute((nx + 7) / 8,(ny + 7) / 8,(nz + 7) / 8);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return m_texB;
}

void SmoothPass::init(const std::string& compPath) {
	compShader.compileCompute(compPath);
}

void SmoothPass::release() {
	if (m_texA != 0) {
		glDeleteTextures(1, &m_texA);
		m_texA = 0;
	}

	if (m_texB != 0) {
		glDeleteTextures(1, &m_texB);
		m_texB = 0;
	}

	m_nx = 0;
	m_ny = 0;
	m_nz = 0;
}
