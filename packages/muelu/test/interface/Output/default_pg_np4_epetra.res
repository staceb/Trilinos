Level 0
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: type = Gauss-Seidel   [unused]
 
Level 1
 Prolongator smoothing (PG-AMG) (MueLu::PgPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::UncoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    aggregation: drop tol = 0   [default]
    aggregation: Dirichlet threshold = 0   [default]
    aggregation: drop scheme = classical   [default]
    lightweight wrap = 1
    
   aggregation: mode = new   [unused]
   aggregation: max agg size = -1   [default]
   aggregation: min agg size = 2   [default]
   aggregation: max selected neighbors = 0   [default]
   aggregation: ordering = natural   [default]
   aggregation: enable phase 1 = 1   [default]
   aggregation: enable phase 2a = 1   [default]
   aggregation: enable phase 2b = 1   [default]
   aggregation: enable phase 3 = 1   [default]
   aggregation: preserve Dirichlet points = 0   [default]
   UseOnePtAggregationAlgorithm = 0   [default]
   UsePreserveDirichletAggregationAlgorithm = 0   [default]
   UseUncoupledAggregationAlgorithm = 1   [default]
   UseMaxLinkAggregationAlgorithm = 1   [default]
   UseIsolatedNodeAggregationAlgorithm = 1   [default]
   UseEmergencyAggregationAlgorithm = 1   [default]
   OnePt aggregate map name =    [default]
   
   Build (MueLu::AmalgamationFactory)
   [empty list]
   
   Nullspace factory (MueLu::NullspaceFactory)
   Fine level nullspace = Nullspace
   
   Build (MueLu::CoarseMapFactory)
   Striding info = {}   [default]
   Strided block id = -1   [default]
   Domain GID offsets = {0}   [default]
   
  [empty list]
  
  PgPFactory::ComputeRowBasedOmega (MueLu::PgPFactory)
 Minimization norm = 2   [default]
 ReUseRowBasedOmegas = 0   [default]
 
 Call prolongator factory for calculating restrictor (MueLu::GenericRFactory)
  Prolongator smoothing (PG-AMG) (MueLu::PgPFactory)
   PgPFactory::ComputeRowBasedOmega (MueLu::PgPFactory)
  Minimization norm = 2   [default]
  ReUseRowBasedOmegas = 0   [default]
  
 [empty list]
 
 Computing Ac (MueLu::RAPFactory)
 transpose: use implicit = 0   [default]
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 CheckMainDiagonal = 0   [default]
 RepairMainDiagonal = 0   [default]
 
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: type = Gauss-Seidel   [unused]
 
Level 2
 Prolongator smoothing (PG-AMG) (MueLu::PgPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::UncoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    aggregation: drop tol = 0   [default]
    aggregation: Dirichlet threshold = 0   [default]
    aggregation: drop scheme = classical   [default]
    lightweight wrap = 1
    
   aggregation: mode = new   [unused]
   aggregation: max agg size = -1   [default]
   aggregation: min agg size = 2   [default]
   aggregation: max selected neighbors = 0   [default]
   aggregation: ordering = natural   [default]
   aggregation: enable phase 1 = 1   [default]
   aggregation: enable phase 2a = 1   [default]
   aggregation: enable phase 2b = 1   [default]
   aggregation: enable phase 3 = 1   [default]
   aggregation: preserve Dirichlet points = 0   [default]
   UseOnePtAggregationAlgorithm = 0   [default]
   UsePreserveDirichletAggregationAlgorithm = 0   [default]
   UseUncoupledAggregationAlgorithm = 1   [default]
   UseMaxLinkAggregationAlgorithm = 1   [default]
   UseIsolatedNodeAggregationAlgorithm = 1   [default]
   UseEmergencyAggregationAlgorithm = 1   [default]
   OnePt aggregate map name =    [default]
   
   Build (MueLu::AmalgamationFactory)
   [empty list]
   
   Nullspace factory (MueLu::NullspaceFactory)
   Fine level nullspace = Nullspace
   
   Build (MueLu::CoarseMapFactory)
   Striding info = {}   [default]
   Strided block id = -1   [default]
   Domain GID offsets = {0}   [default]
   
  [empty list]
  
  PgPFactory::ComputeRowBasedOmega (MueLu::PgPFactory)
 Minimization norm = 2   [default]
 ReUseRowBasedOmegas = 0   [default]
 
 Call prolongator factory for calculating restrictor (MueLu::GenericRFactory)
  Prolongator smoothing (PG-AMG) (MueLu::PgPFactory)
   PgPFactory::ComputeRowBasedOmega (MueLu::PgPFactory)
  Minimization norm = 2   [default]
  ReUseRowBasedOmegas = 0   [default]
  
 [empty list]
 
 Computing Ac (MueLu::RAPFactory)
 transpose: use implicit = 0   [default]
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 CheckMainDiagonal = 0   [default]
 RepairMainDiagonal = 0   [default]
 
 Setup Smoother (MueLu::AmesosSmoother{type = Superlu})
 presmoother -> 
  [empty list]
 

--------------------------------------------------------------------------------
---                            Multigrid Summary                             ---
--------------------------------------------------------------------------------
Number of levels    = 3
Operator complexity = 1.45

matrix rows    nnz  nnz/row procs
A 0    9999  29995     3.00  4
A 1    3335  10015     3.00  4
A 2    1112   3340     3.00  4

Smoother (level 0) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}

Smoother (level 1) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}

Smoother (level 2) pre  : MueLu::AmesosSmoother{type = Superlu}
Smoother (level 2) post : no smoother
