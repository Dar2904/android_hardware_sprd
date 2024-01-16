/**
 * @file   morpho_face_finder.h
 * @brief  �摜������畔���̏��擾
 *
 * Copyright (C) 2007-2010 Morpho, Inc.
 */

#ifndef MORPHO_FACE_FINDER_H
#define MORPHO_FACE_FINDER_H

#include "morpho_api.h"
#include "morpho_error.h"
#include "morpho_image_data.h"
#include "morpho_rect_int.h"
#include "morpho_point_int.h"

#define MORPHO_FACE_SOLID_VER "FaceSolid Ver.4.1.3.ZTE0 2012/03/21"

#ifdef __cplusplus
extern "C" {
#endif

enum {
/**<   0�� �炪������̏�ԁA�Ⴊ������y���W�������� */
    MORPHO_FACE_FINDER_INCLINATION_0   = 0x00000001, 
/**< 90��  �����v���� */
    MORPHO_FACE_FINDER_INCLINATION_90  = 0x00000002, 
/**< 180�� �����v���� */
    MORPHO_FACE_FINDER_INCLINATION_180 = 0x00000004, 
/**< 270�� �����v���� */
    MORPHO_FACE_FINDER_INCLINATION_270 = 0x00000008, 
    MORPHO_FACE_FINDER_INCLINATION_NUM = 4,
};

/**
 * ���o���ꂽ��̈ʒu���A�P�x���AID���i���������ԁj
 * ��ێ����邽�߂̍\����.
 */
typedef struct{
    int face_id;
    int sx;
    int sy;
    int ex;
    int ey;
    int brightness;
    int angle;
    int smile_level;
    int blink_level;
}morpho_FaceRect;

/**
 * ���o������̃p�[�c����ێ����邽�߂̍\����.
 */
typedef struct {
    morpho_PointInt right_pupil; /**< �E���E */
    morpho_PointInt left_pupil; /**< �����E */
    morpho_PointInt right_mouth_corner; /**< ���E�[ */
    morpho_PointInt left_mouth_corner; /**< �����[ */
} morpho_FaceParts;


enum {
    /* ���� */
    MORPHO_FACE_REG_RIGHT_EYE_PUPIL = 0,
    MORPHO_FACE_REG_LEFT_EYE_PUPIL,

    /* �� */
    MORPHO_FACE_REG_RIGHT_MOUTH_CORNER,
    MORPHO_FACE_REG_LEFT_MOUTH_CORNER,

    /* ���сF�E���� */
    MORPHO_FACE_REG_OUTER_END_OF_RIGHT_EYE_BROW,
    MORPHO_FACE_REG_INNER_END_OF_RIGHT_EYE_BROW,
    MORPHO_FACE_REG_INNER_END_OF_LEFT_EYE_BROW,
    MORPHO_FACE_REG_OUTER_END_OF_LEFT_EYE_BROW,

    /* �ځF�E���� */
    MORPHO_FACE_REG_RIGHT_TEMPLE,
    MORPHO_FACE_REG_OUTER_CORNER_OF_RIGHT_EYE,
    MORPHO_FACE_REG_INNER_CORNER_OF_RIGHT_EYE,
    MORPHO_FACE_REG_INNER_CORNER_OF_LEFT_EYE,
    MORPHO_FACE_REG_OUTER_CORNER_OF_LEFT_EYE,
    MORPHO_FACE_REG_LEFT_TEMPLE,
    
    /* �@ */
    MORPHO_FACE_REG_TIP_OF_NOSE,
    MORPHO_FACE_REG_RIGHT_NOSTRIL,
    MORPHO_FACE_REG_LEFT_NOSTRIL,

    /* �� */
    MORPHO_FACE_REG_CENTER_POINT_ON_OUTER_EDGE_OF_UPPER_LIP,
    MORPHO_FACE_REG_CENTER_POINT_ON_OUTER_EDGE_OF_LOWER_LIP,

    /* �{ */
    MORPHO_FACE_REG_TIP_OF_CHIN,

    
    /**** EXTRA Face Parts ****/
    MORPHO_FACE_REG_CENTER_POINT_ON_OUTER_EDGE_OF_RIGHT_EYE_BROW,
    MORPHO_FACE_REG_CENTER_POINT_ON_OUTER_EDGE_OF_LEFT_EYE_BROW,
    MORPHO_FACE_REG_CONTER_POINT_ON_UPPER_EDGE_OF_RIGHT_EYELID,
    MORPHO_FACE_REG_CONTER_POINT_ON_LOWER_EDGE_OF_RIGHT_EYELID,
    MORPHO_FACE_REG_CONTER_POINT_ON_UPPER_EDGE_OF_LEFT_EYELID,
    MORPHO_FACE_REG_CONTER_POINT_ON_LOWER_EDGE_OF_LEFT_EYELID,

    MORPHO_FACE_REG_OF_UPPER_POINT_ON_RIGHT_FACE_LINE,
    MORPHO_FACE_REG_OF_LOWER_POINT_ON_RIGHT_FACE_LINE,
    MORPHO_FACE_REG_OF_UPPER_POINT_ON_LEFT_FACE_LINE,
    MORPHO_FACE_REG_OF_LOWER_POINT_ON_LEFT_FACE_LINE,

    MORPHO_FACE_REG_LIP0,
    MORPHO_FACE_REG_LIP1,
    MORPHO_FACE_REG_LIP2,
    MORPHO_FACE_REG_LIP3,
    MORPHO_FACE_REG_LIP4,
    MORPHO_FACE_REG_LIP5,
    MORPHO_FACE_REG_LIP6,
    MORPHO_FACE_REG_LIP7,
    MORPHO_FACE_REG_LIP8,
    MORPHO_FACE_REG_LIP9,
    
    MORPHO_FACE_REG_NUM_FACE_PARTS
};


/**
 * �猟�o��
 */
typedef struct{
    void * p;
}morpho_FaceFinder;


/**
 * �g�p���郁�����ʂ�Ԃ��܂�.
 * 
 * @param[in] width ���͉摜�̕�
 * @param[in] height ���͉摜�̍���
 * @return �K�v�ȃ������T�C�Y(byte)
 * 
 */
MORPHO_API(int)
morpho_FaceFinder_getBufferSize(int width , int height);

/**
 * �猟�o������������܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_buffer     �q�[�v�̈�ւ̃|�C���^
 * @param[in] buffer_size  �g�p�\�q�[�v�̈�̃T�C�Y
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_initialize( morpho_FaceFinder * p_facefinder , void * p_buffer, int buffer_size);

/**
 * �猟�o����J�n���܂�.
 * ����ȍ~�A���[�h�A�ő匟�o�l���̑I�������邱�Ƃ͂ł��܂���.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @return �G���[�R�[�h(morpho_error.h)
 */

MORPHO_API(int)
morpho_FaceFinder_start( morpho_FaceFinder * p_facefinder );

/**
 * �Ώۉ摜�ɑ΂��āA�猟�o�������J�n���܂�.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_image      �摜�ւ̃|�C���^
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_detectStart( morpho_FaceFinder * p_facefinder, morpho_ImageData * p_image );

/**
 * �Ώۉ摜�ɑ΂��āA�猟�o���������s���܂�.
 *
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[out] p_progress      ���o������(0-32768)
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_detect( morpho_FaceFinder * p_facefinder, int *p_progress);

/**
 * �Ώۉ摜�ɑ΂��āA�猟�o�������I�����܂�.
 *
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_image      �摜�ւ̃|�C���^
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_detectEnd( morpho_FaceFinder * p_facefinder );

/**
 * ���o���ꂽ��̋�`�������Z�b�g���܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_detectReset( morpho_FaceFinder * p_facefinder );

/**
 * ���o���ꂽ��̏����擾���܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[out] p_face_num  ���o���ꂽ�l��
 * @param[out] p_faces     ���o���ꂽ��Ɋւ�����
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getFaces( morpho_FaceFinder * p_facefinder, int * p_face_num, morpho_FaceRect * p_faces );

/**
 * �猟�o���j�����܂�.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_finalize( morpho_FaceFinder * p_facefinder );

/**
 * ���o����摜�̃t�H�[�}�b�g���w�肵�܂�.
 * ���ݑΉ����Ă���t�H�[�}�b�g�͈ȉ��̒ʂ�ł��B
 * YUV422_YUYV, YUV422_YVYU, YUV422_YYUV, YUV422_YYVU,
 * YUV422_VYUY, YUV422_UYVY, YUV422_VUYY, YUV422_UVYY,
 * YUV422_PLANAR, YUV422_SEMIPLANAR,
 * YUV420_PLANAR, YUV420_SEMIPLANAR,
 * YVU422_SEMIPLANAR, YVU420_SEMIPLANAR,
 * RGB565, RGB888
 * ���ȉ��́A�Ί�/�p�[�c���o�͖��Ή�
 * YUV444_YUV, YUV444_YVU, YUV444_UYV,
 * YUV444_VYU, YUV444_UVY, YUV444_VUY
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_format     �摜�t�H�[�}�b�g
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setImageFormat( morpho_FaceFinder * p_facefinder, const char * p_format );

/**
 * �ݒ肳��Ă���摜�̃t�H�[�}�b�g���擾���܂�.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[out] p_format     �摜�t�H�[�}�b�g
 */
MORPHO_API(int)
morpho_FaceFinder_getImageFormat( morpho_FaceFinder * p_facefinder, const char ** p_format );

/**
 * ���o����摜�̃T�C�Y���w�肵�܂�.
 * �{�w���morpho_FaceFinder_getBufferSize()�w�莞�Ɠ��l�̒l���w�肵�Ă�������
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] width     �摜�̕�
 * @param[in] height    �摜�̍���
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setImageSize( morpho_FaceFinder * p_facefinder, int width, int height);

/**
 * ���o����摜�̃T�C�Y���擾���܂�.
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[out] p_width     �摜�̕�
 * @param[out] p_height    �摜�̍���
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getImageSize( morpho_FaceFinder * p_facefinder, int *p_width, int *p_height);

/**
 * ���o����X���̑Ώۊp�x���w�肵�܂�.
 * 
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[in] inclination_type ���o�Ώۊp�x�̃^�C�v
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setInclinationType( morpho_FaceFinder * p_facefinder, int inclination_type );

/**
 * �ݒ肳��Ă��錟�o����X���̑Ώۊp�x���擾���܂�.
 * 
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[in] p_inclination_type ���o�Ώۊp�x�̃^�C�v
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getInclinationType( morpho_FaceFinder * p_facefinder, int *p_inclination_type );

/**
 * ���o�����`�̍ŏ��T�C�Y���w�肵�܂�.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] min_detect_size �ŏ���`�T�C�Y��
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setMinDetectSize( morpho_FaceFinder * p_facefinder, int min_detect_size );

/**
 * ���o�����`�̍ŏ��T�C�Y���擾���܂�.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[out] p_min_detect_size �ŏ���`�T�C�Y��
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getMinDetectSize( morpho_FaceFinder *p_facefinder, int *p_min_detect_size );

/**
 * ���o�����`�̍ő�T�C�Y��ݒ肵�܂�.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] max_detect_size �ŏ���`�T�C�Y��
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setMaxDetectSize( morpho_FaceFinder * p_facefinder, int max_detect_size );

/**
 * ���o�����`�̍ő�T�C�Y���擾���܂�.
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[out] p_max_detect_size �ŏ���`�T�C�Y��
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getMaxDetectSize( morpho_FaceFinder *p_facefinder, int *p_max_detect_size );

/**
 * ����擾���A����̃\�[�g���@���w�肵�܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] sort_rule    �\�[�g���@
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */

enum {
    MORPHO_FACE_FINDER_SORT_RULE_NONE = 0, /**< �\�[�g�Ȃ� */
    MORPHO_FACE_FINDER_SORT_RULE_ASCEND_FACE_SIZE, /**< ���`�T�C�Y���������� */
    MORPHO_FACE_FINDER_SORT_RULE_DESCEND_FACE_SIZE, /**< ���`�T�C�Y���������� */
    MORPHO_FACE_FINDER_SORT_RULE_ASCEND_FACE_ID, /**< ��ID���������� */
    MORPHO_FACE_FINDER_SORT_RULE_DESCEND_FACE_ID, /**< ��ID���傫���� */
    MORPHO_FACE_FINDER_SORT_RULE_NUM,
};

MORPHO_API(int)
morpho_FaceFinder_setSortRule( morpho_FaceFinder * p_facefinder, int sort_rule );

/**
 * ����擾���A����̃\�[�g���@���擾���܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[out] p_sort_rule �\�[�g���@
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */

MORPHO_API(int)
morpho_FaceFinder_getSortRule( morpho_FaceFinder * p_facefinder, int *p_sort_rule );

/**
 * �ő匟�o�l�����w�肵�܂�
 * 
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[in] max_face_num �ő匟�o�l���i�ݒ�\�͈́F 1�`MORPHO_FACE_FINDER_MAX_NUM_FACE�j
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */
enum {
    MORPHO_FACE_FINDER_MAX_FACE_NUM = 20,
};

MORPHO_API(int)
morpho_FaceFinder_setMaxFaceNum( morpho_FaceFinder *p_facefinder, int max_face_num );

/**
 * �w�肳��Ă���ő匟�o�l�����擾���܂�
 * 
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[out] p_max_face_num �ő匟�o�l��
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getMaxFaceNum( morpho_FaceFinder *p_facefinder, int *p_max_face_num );

/**
 * �i�]���p�j
 * �T���͈͂̋�`����͉摜�̍��W�Őݒ肵�܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_search_rect �����͈͂̋�`�i���͉摜���̍��W���w��j
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setSearchAreaRect( morpho_FaceFinder * p_facefinder, morpho_RectInt *p_search_rect);

/**
 * �i�]���p�j
 * �ݒ肳��Ă���T���͈͂̋�`���擾���܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_search_rect �����͈͂̋�`�i���͉摜���̍��W���w��j
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getSearchAreaRect( morpho_FaceFinder * p_facefinder, morpho_RectInt *p_search_rect);

/**
 * ����ӌ����J�E���g�̎w��
 * 
 * ���o���ꂽ�����8������1Pixel���炵�Č�����,�w�萔�ȏ�̊�̌��o���Ȃ���Ό댟�o�Ƃ݂Ȃ�
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[in] false_suppression_level ���ӌ����J�E���g�i�ݒ�\�͈́F 0�`8�j
 * 
 * �f�t�H���g:1
 * 0:�������s��Ȃ�
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setFalseDetectionSuppressionLevel( morpho_FaceFinder * p_facefinder, int false_suppression_level );

/**
 * ����ӌ����J�E���g�̎擾
 * 
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[out] p_false_suppression_level ���ӌ����J�E���g
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getFalseDetectionSuppressionLevel( morpho_FaceFinder * p_facefinder, int *p_false_suppression_level );

/**
 * TrackValidateStep�̎w��
 * �g���b�L���O���ɍČ��������{����p�x
 * morpho_FaceFinder_detectStart���ݒ�l�񐔌Ăяo�����Ԃ́A�ȈՓI�Ȋ猟�o�����s�����
 *
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[in] light_track_num �i�ݒ�\�͈́F 0�`30�j
 * 
 * �f�t�H���g:30
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_setLightTrackNum( morpho_FaceFinder * p_facefinder, int light_track_num );

/**
 * TrackValidateStep�̎擾
 * 
 *
 * @param[in] p_facefinder  �猟�o��ւ̃|�C���^
 * @param[out] p_light_track_num
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_getLightTrackNum( morpho_FaceFinder * p_facefinder, int *p_light_track_num );

/**
 * �w�肵����ID�̃p�[�c�����擾���܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_image      ���͉摜
 * @param[in] p_rect       ����
 * @param[out] p_parts     ��p�[�c���
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_detectFaceParts( morpho_FaceFinder *p_facefinder, morpho_ImageData *p_image, morpho_FaceRect *p_rect, morpho_FaceParts *p_parts );

/**
    * �w�肵����ID�̃p�[�c���(�ڍ�)���擾���܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] p_image      ���͉摜
 * @param[in] p_rect       ����
 * @param[out] p_points    ��p�[�c���W
 * @param[in]  point_num   ���W(p_points)�z��
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */
MORPHO_API(int)
morpho_FaceFinder_detectFacePartsDetail( morpho_FaceFinder *p_facefinder, morpho_ImageData *p_image, morpho_FaceRect *p_rect, morpho_PointInt *p_points, int point_num );

/**
 * ���o���ꂽ��̏Ί�x�𔻒肵�܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] image        ���͉摜�ւ̃|�C���^
 * @param[in] p_face       ���o���ꂽ��Ɋւ�����(�{API�̂݊���͈�����w��)
 * @param[out] p_smile     �Ί�x(0�`255)
 * @return �G���[�R�[�h(morpho_error.h)
 */
 
MORPHO_API(int)
morpho_FaceFinder_detectSmile( morpho_FaceFinder * p_facefinder, morpho_ImageData * p_image, morpho_FaceRect *p_face, int* p_smile);

/**
 * ���o���ꂽ��̏u���x�𔻒肵�܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] image        ���͉摜�ւ̃|�C���^
 * @param[in] p_face       ���o���ꂽ��Ɋւ�����(�{API�̂݊���͈�����w��)
 * @param[out] p_blink     �Ί�x(0�`255)
 * @return �G���[�R�[�h(morpho_error.h)
 */

MORPHO_API(int)
morpho_FaceFinder_detectBlink( morpho_FaceFinder * p_facefinder, morpho_ImageData * p_image, morpho_FaceRect *p_face, int* p_blink);


/**
 * �w������Ŋ炪���o����Ă��邩���肵�܂�
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in] image        ���͉摜�ւ̃|�C���^
 * @param[in] p_face_num  ���o���ꂽ�l��
 * @param[in] p_face       ���o���ꂽ��Ɋւ�����
 * @param[in] shotarea     �B�e���o�͈́i�w��摜�̒��S���牽�������o�͈͂Ƃ��邩�j
 * @param[out] p_out_bool   ���o����(1�F�w��͈͓��A0�F�w��͈͊O)
 * 
 * @return �G���[�R�[�h(morpho_error.h)
 */

 
MORPHO_API(int)
morpho_FaceFinder_isShutterChance( morpho_FaceFinder * p_facefinder, morpho_ImageData * p_image, int face_num, morpho_FaceRect * p_faces,int shotarea, int* p_out_bool);

/**
 * ��̋P�x����l�ilevel�Ō��܂�j�ȉ��̏ꍇ���邳��␳���܂�
 * �f�t�H���g�ł͎g�p�s�̊֐��ł��B
 * 
 * @param[in] p_facefinder �猟�o��ւ̃|�C���^
 * @param[in/out] p_image  ���o�͉摜�ւ̃|�C���^
 * @param[in] p_face_num   ���o���ꂽ�l��
 * @param[in] p_faces      ���o���ꂽ��Ɋւ�����
 * @param[in] level        �t���␳���x��(0-255)
 * @return �G���[�R�[�h(morpho_error.h)
 */

MORPHO_API(int)
morpho_FaceFinder_normalizeColor( morpho_FaceFinder * p_facefinder, morpho_ImageData * p_image, int face_num, morpho_FaceRect * p_faces, int level );

/**
 * �{���C�u�����̃o�[�W���������擾���܂�
 * 
 * @return [out]const char* p_Version:�{���C�u�����̃o�[�W�������
 */
MORPHO_API(const char*)
morpho_FaceFinder_getVersion( void );


#ifdef __cplusplus
}
#endif

#endif /* MORPHO_FACE_FINDER_H */

