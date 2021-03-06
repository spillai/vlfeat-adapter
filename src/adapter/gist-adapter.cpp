/*
 * gist-adapter.cpp
 *
 *  Created on: May 21, 2013
 *      Author: jieshen
 */

#include "gist-adapter.hpp"

#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using std::cerr;
using std::endl;

namespace jieshen
{
    const int GIST_ADAPTER::DEFAULT_ORT[3] = { 8, 8, 4 };

    GIST_ADAPTER::GIST_ADAPTER()
    {
        init();
    }

    GIST_ADAPTER::GIST_ADAPTER(const Mat* img)
    {
        init();
        setImage(img);
    }

    GIST_ADAPTER::~GIST_ADAPTER()
    {
        clear();
    }

    void GIST_ADAPTER::init()
    {
        BASIC_ADAPTER::init();
        init_gist_model();
    }

    void GIST_ADAPTER::init_gist_model()
    {
        init_gist_parameters();

        m_gist_features = NULL;
        m_has_extracted = false;

        m_has_set_image = false;
        m_is_gray = true;
        m_gray_img = NULL;
        m_color_img = NULL;
    }

    void GIST_ADAPTER::init_gist_parameters()
    {
        m_nblock = DEFAULT_NBLOCK_INVALID;
        m_nscale = DEFAULT_NSCALE_INVALID;
        m_orients = NULL;
    }

    void GIST_ADAPTER::clear()
    {
        clear_gist_model();
        clearImage();
    }

    void GIST_ADAPTER::clear_gist_model()
    {
        clear_model_related_data();
        m_nblock = DEFAULT_NBLOCK_INVALID;
        m_nscale = DEFAULT_NSCALE_INVALID;
        if (m_orients)
            utils::myfree(&m_orients);
        m_orients = NULL;
    }

    void GIST_ADAPTER::clear_model_related_data()
    {
        m_has_extracted = false;

        if (m_gist_features)
            utils::myfree(&m_gist_features);
    }

    void GIST_ADAPTER::clearImage()
    {
        m_has_extracted = false;

        clear_gray_image_data();
        if(m_org_img.data)
            m_org_img.release();

        if (m_is_gray)
        {
            if (m_gray_img)
            {
                image_delete(m_gray_img);
                m_gray_img = NULL;
            }
        }
        else
        {
            if (m_color_img)
            {
                color_image_delete(m_color_img);
                m_color_img = NULL;
            }
        }

        m_is_gray = true;
        m_has_set_image = false;
    }

    void GIST_ADAPTER::set_gist_model()
    {
        m_has_extracted = false;

        clear_model_related_data();
    }

    void GIST_ADAPTER::set_gray_image_data()
    {
        BASIC_ADAPTER::set_gray_image_data();

        if (m_org_img.channels() == 1)
            m_is_gray = true;
        else
            m_is_gray = false;

        int m_img_width = m_org_img.cols;
        int m_img_height = m_org_img.rows;

        if (m_is_gray)
        {
            m_gray_img = image_new(m_img_width, m_img_height);
            memcpy(m_gray_img->data, m_gray_data,
                   m_img_width * m_img_height * sizeof(float));
        }
        else
        {
            m_color_img = color_image_new(m_img_width, m_img_height);
            Mat _img;
            m_org_img.convertTo(_img, CV_32FC3);

            for (int y = 0; y < m_img_height; ++y)
            {
                const cv::Vec3f* p_line = _img.ptr<cv::Vec3f>(y);
                int stride = y * m_img_width;

                float* r = m_color_img->c1 + stride;
                float* g = m_color_img->c2 + stride;
                float* b = m_color_img->c3 + stride;

                for (int x = 0; x < m_img_width; ++x)
                {
                    *(r + x) = (p_line + x)->val[2];
                    *(g + x) = (p_line + x)->val[1];
                    *(b + x) = (p_line + x)->val[0];
                }
            }
        }
    }

    void GIST_ADAPTER::setImage(const Mat* img)
    {
        m_has_extracted = false;

        img->copyTo(m_org_img);
        //set_image_data();
        m_has_set_image = true;
    }

    void GIST_ADAPTER::setNBlock(const int nblock)
    {
        if (nblock <= 0)
        {
            cerr << "number of block should be > 0" << endl;
            exit(-1);
        }

        if (nblock == m_nblock)
            return;

        m_nblock = nblock;

        m_has_extracted = false;
    }

    void GIST_ADAPTER::setOrients(const int nort, const int* orts)
    {
        if (nort <= 0 || !orts)
        {
            cerr << "number of orientation should > 0 and please check the array"
                 << endl;
            exit(-1);
        }

        if (nort == m_nscale)
        {
            bool same = true;
            for (int i = 0; same && i < nort; ++i)
                if (orts[i] != m_orients[i])
                    same = false;
            if (same)
                return;
        }

        m_nscale = nort;

        if (m_orients)
            utils::myfree(&m_orients);
        memcpy(m_orients, orts, m_nscale * sizeof(int));

        m_has_extracted = false;
    }

    void GIST_ADAPTER::resetNBlock()
    {
        if (m_nblock == DEFAULT_NBLOCK_INVALID)
            return;
        m_nblock = DEFAULT_NBLOCK_INVALID;
        m_has_extracted = false;
    }

    void GIST_ADAPTER::resetOrients()
    {
        if (m_nscale == DEFAULT_NSCALE_INVALID)
            return;

        m_nscale = DEFAULT_NSCALE_INVALID;
        if (m_orients)
            utils::myfree(&m_orients);
        m_orients = NULL;
        m_has_extracted = false;
    }

    void GIST_ADAPTER::resetGistModel()
    {
        clear_model_related_data();
        m_nblock = DEFAULT_NBLOCK_INVALID;
        m_nscale = DEFAULT_NSCALE_INVALID;
        if (m_orients)
            utils::myfree(&m_orients);
        m_orients = NULL;
    }

    int GIST_ADAPTER::getNBlock() const
    {
        if (m_nblock == DEFAULT_NBLOCK_INVALID)
            return DEFAULT_NBLOCK;
        return m_nblock;
    }

    int GIST_ADAPTER::getNScale() const
    {
        if (m_nscale == DEFAULT_NSCALE_INVALID)
            return DEFAULT_NSCALE;
        return m_nscale;
    }

    const int* GIST_ADAPTER::getNOrientsPerScale() const
    {
        if (m_orients == NULL)
            return DEFAULT_ORT;
        return m_orients;
    }

    int GIST_ADAPTER::getGistFeatureDim() const
    {
        if (!m_has_extracted)
        {
            cerr << "Please call the method extractGistFeature() first" << endl;
            return 0;
        }
        const int nblock = getNBlock();
        const int nscale = getNScale();
        const int* orts = getNOrientsPerScale();

        int dim = nblock * nblock;
        int s = 0;
        for (int i = 0; i < nscale; ++i)
            s += orts[i];
        dim *= s;

        dim *= m_org_img.channels();

        return dim;
    }

    const float* GIST_ADAPTER::getGistFeature() const
    {
        if (!m_has_extracted)
        {
            cerr << "There is no gist feature. Please check the image\\"
                 " and model setting, then call extractGistFeature()"
                 << endl;
            exit(-1);
        }

        return m_gist_features;
    }

    void GIST_ADAPTER::extractGistFeature(vector<float>* descriptor)
    {
        check_image();

        if (m_has_extracted)
            return;

        set_gray_image_data();
        set_gist_model();

        const int* orts = getNOrientsPerScale();

        if (m_is_gray)
            m_gist_features = bw_gist_scaletab(m_gray_img, getNBlock(),
                                               getNScale(), orts);
        else
            m_gist_features = color_gist_scaletab(m_color_img, getNBlock(),
                                                  getNScale(), orts);

        m_has_extracted = true;

        if (descriptor)
        {
            int dim = getGistFeatureDim();
            descriptor->resize(dim, 0.0);
            std::copy(m_gist_features, m_gist_features + dim,
                      descriptor->begin());

            //std::cerr << dim << std::endl;
        }
    }

    string GIST_ADAPTER::info() const
    {
        using std::cout;
        using std::endl;
        std::string info = "=====GIST Settings=====\n";

        const int nblock = getNBlock();
        const int nscale = getNScale();
        const int* orts = getNOrientsPerScale();

        info += "NBlock:       " + utils::myitoa(nblock) + "\n";
        info += "NScale:       " + utils::myitoa(nscale) + "\n";
        info += "NOrient:      ";

        for (int i = 0; i < nscale; ++i)
            info += utils::myitoa(orts[i]) + " ";

        info += "\n";

        info += "\n-----Image Info-----\n";
        info += "Size: " + utils::myitoa(m_org_img.cols) + " * "
                + utils::myitoa(m_org_img.rows) + "\n";

        info += "Feature Size: " + utils::myitoa(getGistFeatureDim()) + "\n";

        return info;
    }
}
