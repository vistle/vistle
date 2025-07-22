! This example creates a zone with a single polyhedral cell.

program pyramid
    use iso_c_binding
    implicit none
    include 'tecio.f90'

    POINTER(NullPtr, NULL)
    INTEGER(c_int32_t) :: NULL(*)
    INTEGER(c_int32_t) :: FileType   = 0   ! 0 for full file
    INTEGER(c_int32_t) :: FileFormat = 0   ! 0 == PLT, 1 == SZPLT Only PLT is currently supported for polyhedral zones
    INTEGER(c_int32_t) :: Debug      = 0
    INTEGER(c_int32_t) :: VIsdouble  = 1
    INTEGER(c_int32_t) :: I          = 0      ! use to check return codes
    INTEGER(c_int32_t) :: ZoneType   = 7      ! 7 for FEPolyhedron
    INTEGER(c_int32_t) :: NumNodes   = 5      ! number of unique nodes
    INTEGER(c_int32_t) :: NumElems   = 1      ! number of elements
    INTEGER(c_int64_t) :: NumFaces   = 5      ! number of unique faces
    INTEGER(c_int32_t) :: NumFaces32 = 5      ! needed for the call to TECPOLYFACE142
    INTEGER(c_int32_t) :: ICellMax   = 0      ! Not Used, set to zero
    INTEGER(c_int32_t) :: JCellMax   = 0      ! Not Used, set to zero
    INTEGER(c_int32_t) :: KCellMax   = 0      ! Not Used, set to zero
    REAL(c_double)     :: SolTime    = 12.65d0 ! solution time  
    INTEGER(c_int32_t) :: StrandID   = 0      ! static zone    
    INTEGER(c_int32_t) :: unused     = 0      ! ParentZone is no longer used
    INTEGER(c_int32_t) :: IsBlock    = 1      ! block format 
    INTEGER(c_int32_t) :: NFConns    = 0      ! not used for FEPolyhedron zones                             
    INTEGER(c_int32_t) :: FNMode     = 0      ! not used for FEPolyhedron zones
    INTEGER(c_int32_t) :: ShrConn    = 0
    INTEGER(c_int64_t) :: NumFaceNodes = 16
    INTEGER(c_int32_t) :: NumBConns    = 0  ! No Boundary Connections
    INTEGER(c_int32_t) :: NumBItems    = 0  ! No Boundary Items
    REAL(c_double), dimension(5) :: X
    REAL(c_double), dimension(5) :: Y
    REAL(c_double), dimension(5) :: Z
    INTEGER(c_int32_t), dimension(5)  :: FaceNodeCounts
    INTEGER(c_int32_t), dimension(16) :: FaceNodes
    INTEGER(c_int32_t), dimension(5)  :: FaceRightElems
    INTEGER(c_int32_t), dimension(5)  :: FaceLeftElems
    
    NullPtr = 0

    I = TECINI142('Pyramid' // char(0), &        ! Data Set Title
                  'X Y Z' // char(0), &          ! Variable List
                  'pyramidf90.plt' // char(0), & ! File Name
                  '.' // char(0), &              ! Scratch Directory
                  FileFormat, &
                  FileType, &
                  Debug, &
                  VIsdouble)

    ! The number of face nodes in the zone.  This example creates
    ! a zone with a single pyramidal cell.  This cell has four
    ! triangular faces and one rectangular face, yielding a total
    ! of 16 face nodes.

    I = TECPOLYZNE142('Polyhedral Zone (Octahedron)'//char(0), &
                      ZoneType, &
                      NumNodes, &
                      NumElems, &
                      NumFaces, &
                      NumFaceNodes, &
                      SolTime, &
                      StrandID, &
                      unused, &
                      NumBConns, &
                      NumBItems, &
                      NULL, & ! PassiveVarArray
                      NULL, & ! ValueLocArray
                      NULL, & ! VarShareArray
                      ShrConn)

    ! Initialize arrays of nodal data
    X(1) = 0
    Y(1) = 0
    Z(1) = 0

    X(2) = 1
    Y(2) = 1
    Z(2) = 2

    X(3) = 2
    Y(3) = 0
    Z(3) = 0

    X(4) = 2
    Y(4) = 2
    Z(4) = 0

    X(5) = 0
    Y(5) = 2
    Z(5) = 0

    ! Write the data.
    I = TECDATD142(NumNodes, X)
    I = TECDATD142(NumNodes, Y)
    I = TECDATD142(NumNodes, Z)

    ! Define the Face Nodes.
    !
    ! The FaceNodes array is used to indicate which nodes define
    ! which face. As mentioned earlier, the number of the nodes is
    ! implicitly defined by the order in which the nodal data is
    ! provided.  The first value of each nodal variable describes
    ! node 1, the second value describes node 2, and so on.
    !
    ! The face numbering is implicitly defined.  Because there are
    ! two nodes in each face, the first two nodes provided define
    ! face 1, the next two define face 2 and so on.  If there was
    ! a variable number of nodes used to define the faces, the
    ! array would be more complicated.

    ! The first four faces are triangular, i.e. have three nodes.
    ! The fifth face is rectangular, i.e. has four nodes.
    FaceNodeCounts(1) = 3
    FaceNodeCounts(2) = 3
    FaceNodeCounts(3) = 3
    FaceNodeCounts(4) = 3
    FaceNodeCounts(5) = 4

    ! Face Nodes for Face 1
    FaceNodes(1)  = 1
    FaceNodes(2)  = 2
    FaceNodes(3)  = 3

    ! Face Nodes for Face 2
    FaceNodes(4)  = 3
    FaceNodes(5)  = 2
    FaceNodes(6)  = 4

    ! Face Nodes for Face 3
    FaceNodes(7)  = 5
    FaceNodes(8)  = 2
    FaceNodes(9)  = 4

    ! Face Nodes for Face 4
    FaceNodes(10)  = 1
    FaceNodes(11) = 2
    FaceNodes(12) = 5

    ! Face Nodes for Face 5
    FaceNodes(13) = 1
    FaceNodes(14) = 5
    FaceNodes(15) = 4
    FaceNodes(16) = 3

    ! Define the right and left elements of each face.
    !
    ! The last step for writing out the polyhedral data is to
    ! define the right and left neighboring elements for each
    ! face.  The neighboring elements can be determined using the
    ! right-hand rule.  For each face, place your right-hand along
    ! the face which your fingers pointing the direction of
    ! incrementing node numbers (i.e. from node 1 to node 2).
    ! Your right thumb will point towards the right element the
    ! element on the other side of your hand is the left element.
    !
    ! The number zero is used to indicate that there isn't an
    ! element on that side of the face.
    !
    ! Because of the way we numbered the nodes and faces, the
    ! right element for every face is the element itself
    ! (element 1) and the left element is "no-neighboring element"
    ! (element 0).

    FaceLeftElems(1) = 1
    FaceLeftElems(2) = 1
    FaceLeftElems(3) = 0
    FaceLeftElems(4) = 0
    FaceLeftElems(5) = 0

    FaceRightElems(1) = 0
    FaceRightElems(2) = 0
    FaceRightElems(3) = 1
    FaceRightElems(4) = 1
    FaceRightElems(5) = 1
    
    ! Write the face map (created above) using TECPOLYFACE142.
    
    I = TECPOLYFACE142(NumFaces32, &
                       FaceNodeCounts, &  ! The face node counts array
                       FaceNodes, &       ! The face nodes array
                       FaceLeftElems, &   ! The left elements array 
                       FaceRightElems)    ! The right elements array

    I = TECEND142()
end program pyramid
! DOCEND
