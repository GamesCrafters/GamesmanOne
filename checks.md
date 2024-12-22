Checks: >
  -*,                                # Start by disabling all checks
  bugprone-*,
  clang-analyzer-*,
  -clang-analyzer-security.insecureAPI.*,
  concurrency-*,
  cppcoreguidelines-*,
  -cppcoreguidelines-avoid-non-const-global-variables,
  -cppcoreguidelines-init-variables,
  google-*, 
  misc-*
  modernize-*,
  mpi-*,
  openmp-*,
  performance-*,
  portability-*

HeaderFilterRegex: '.*'  # Analyze all files including headers
FormatStyle: "Google"    # Use Google code formatting style
