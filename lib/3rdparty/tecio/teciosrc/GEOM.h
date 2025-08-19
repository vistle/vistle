 #if defined EXTERN
 #undef EXTERN
 #endif
 #if defined ___1616
 #define EXTERN
 #else
 #define EXTERN extern
 #endif
 #define VALID_RECTANGLE_COORDSYS(___3918) \
 (((___3918)==CoordSys_Frame) || \
 ((___3918)==CoordSys_Grid))
 #define VALID_SQUARE_COORDSYS(___3918) VALID_RECTANGLE_COORDSYS((___3918))
 #define VALID_ELLIPSE_COORDSYS(___3918) VALID_RECTANGLE_COORDSYS((___3918))
 #define VALID_CIRCLE_COORDSYS(___3918) VALID_ELLIPSE_COORDSYS((___3918))
 #define VALID_IMAGE_COORDSYS(___3918) (VALID_RECTANGLE_COORDSYS((___3918) || ___3918 == CoordSys_Grid3D))
 #define VALID_LINESEG_COORDSYS(___3918) \
 (((___3918)==CoordSys_Frame) || \
 ((___3918)==CoordSys_Grid)  || \
 ((___3918)==CoordSys_Grid3D))
 #define VALID_GEOM_COORDSYS(___3918) \
 (((___3918)==CoordSys_Frame) || \
 ((___3918)==CoordSys_Grid)  || \
 ((___3918)==CoordSys_Grid3D))
 #define GEOM_USES_V3(___1553) (___1553->___3165 == CoordSys_Grid3D && ___1553->___1650 == GeomType_LineSegs)
 #define VALID_GEOM_TYPE(___1648) \
 ( VALID_ENUM((___1648),GeomType_e) && \
 (___1648)!=GeomType_LineSegs3D )
 #define VALID_GEOM_FIELD_DATA_TYPE(___904) \
 ( ( (___904) == FieldDataType_Float ) || \
 ( (___904) == FieldDataType_Double ) )
