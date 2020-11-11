# Ensures passed in version values are supported.
function check-version-numbers() {
  CHECK_VERSION_MAJOR=$1
  CHECK_VERSION_MINOR=$2

  if [[ $CHECK_VERSION_MAJOR -lt $VECTRUM_MIN_VERSION_MAJOR ]]; then
    exit 1
  fi
  if [[ $CHECK_VERSION_MAJOR -gt $VECTRUM_MAX_VERSION_MAJOR ]]; then
    exit 1
  fi
  if [[ $CHECK_VERSION_MAJOR -eq $VECTRUM_MIN_VERSION_MAJOR ]]; then
    if [[ $CHECK_VERSION_MINOR -lt $VECTRUM_MIN_VERSION_MINOR ]]; then
      exit 1
    fi
  fi
  if [[ $CHECK_VERSION_MAJOR -eq $VECTRUM_MAX_VERSION_MAJOR ]]; then
    if [[ $CHECK_VERSION_MINOR -gt $VECTRUM_MAX_VERSION_MINOR ]]; then
      exit 1
    fi
  fi
  exit 0
}


# Handles choosing which VECTRUM directory to select when the default location is used.
function default-vectrum-directories() {
  REGEX='^[0-9]+([.][0-9]+)?$'
  ALL_VECTRUM_SUBDIRS=()
  if [[ -d ${HOME}/vectrum ]]; then
    ALL_VECTRUM_SUBDIRS=($(ls ${HOME}/vectrum | sort -V))
  fi
  for ITEM in "${ALL_VECTRUM_SUBDIRS[@]}"; do
    if [[ "$ITEM" =~ $REGEX ]]; then
      DIR_MAJOR=$(echo $ITEM | cut -f1 -d '.')
      DIR_MINOR=$(echo $ITEM | cut -f2 -d '.')
      if $(check-version-numbers $DIR_MAJOR $DIR_MINOR); then
        PROMPT_VECTRUM_DIRS+=($ITEM)
      fi
    fi
  done
  for ITEM in "${PROMPT_VECTRUM_DIRS[@]}"; do
    if [[ "$ITEM" =~ $REGEX ]]; then
      VECTRUM_VERSION=$ITEM
    fi
  done
}


# Prompts or sets default behavior for choosing VECTRUM directory.
function vectrum-directory-prompt() {
  if [[ -z $VECTRUM_DIR_PROMPT ]]; then
    default-vectrum-directories;
    echo 'No VECTRUM location was specified.'
    while true; do
      if [[ $NONINTERACTIVE != true ]]; then
        if [[ -z $VECTRUM_VERSION ]]; then
          echo "No default VECTRUM installations detected..."
          PROCEED=n
        else
          printf "Is VECTRUM installed in the default location: $HOME/vectrum/$VECTRUM_VERSION (y/n)" && read -p " " PROCEED
        fi
      fi
      echo ""
      case $PROCEED in
        "" )
          echo "Is VECTRUM installed in the default location?";;
        0 | true | [Yy]* )
          break;;
        1 | false | [Nn]* )
          if [[ $PROMPT_VECTRUM_DIRS ]]; then
            echo "Found these compatible VECTRUM versions in the default location."
            printf "$HOME/vectrum/%s\n" "${PROMPT_VECTRUM_DIRS[@]}"
          fi
          printf "Enter the installation location of VECTRUM:" && read -e -p " " VECTRUM_DIR_PROMPT;
          VECTRUM_DIR_PROMPT="${VECTRUM_DIR_PROMPT/#\~/$HOME}"
          break;;
        * )
          echo "Please type 'y' for yes or 'n' for no.";;
      esac
    done
  fi
  export VECTRUM_INSTALL_DIR="${VECTRUM_DIR_PROMPT:-${HOME}/vectrum/${VECTRUM_VERSION}}"
}


# Prompts or default behavior for choosing VECTRUM.CDT directory.
function cdt-directory-prompt() {
  if [[ -z $CDT_DIR_PROMPT ]]; then
    echo 'No VECTRUM.CDT location was specified.'
    while true; do
      if [[ $NONINTERACTIVE != true ]]; then
        printf "Is VECTRUM.CDT installed in the default location? /usr/local/vectrum.cdt (y/n)" && read -p " " PROCEED
      fi
      echo ""
      case $PROCEED in
        "" )
          echo "Is VECTRUM.CDT installed in the default location?";;
        0 | true | [Yy]* )
          break;;
        1 | false | [Nn]* )
          printf "Enter the installation location of VECTRUM.CDT:" && read -e -p " " CDT_DIR_PROMPT;
          CDT_DIR_PROMPT="${CDT_DIR_PROMPT/#\~/$HOME}"
          break;;
        * )
          echo "Please type 'y' for yes or 'n' for no.";;
      esac
    done
  fi
  export CDT_INSTALL_DIR="${CDT_DIR_PROMPT:-/usr/local/vectrum.cdt}"
}


# Ensures VECTRUM is installed and compatible via version listed in tests/CMakeLists.txt.
function node-version-check() {
  INSTALLED_VERSION=$(echo $($VECTRUM_INSTALL_DIR/bin/node --version))
  INSTALLED_VERSION_MAJOR=$(echo $INSTALLED_VERSION | cut -f1 -d '.' | sed 's/v//g')
  INSTALLED_VERSION_MINOR=$(echo $INSTALLED_VERSION | cut -f2 -d '.' | sed 's/v//g')

  if [[ -z $INSTALLED_VERSION_MAJOR || -z $INSTALLED_VERSION_MINOR ]]; then
    echo "Could not determine VECTRUM version. Exiting..."
    exit 1;
  fi

  if $(check-version-numbers $INSTALLED_VERSION_MAJOR $INSTALLED_VERSION_MINOR); then
    if [[ $INSTALLED_VERSION_MAJOR -gt $VECTRUM_SOFT_MAX_MAJOR ]]; then
      echo "Detected VECTRUM version is greater than recommended soft max: $VECTRUM_SOFT_MAX_MAJOR.$VECTRUM_SOFT_MAX_MINOR. Proceed with caution."
    fi
    if [[ $INSTALLED_VERSION_MAJOR -eq $VECTRUM_SOFT_MAX_MAJOR && $INSTALLED_VERSION_MINOR -gt $VECTRUM_SOFT_MAX_MINOR ]]; then
      echo "Detected VECTRUM version is greater than recommended soft max: $VECTRUM_SOFT_MAX_MAJOR.$VECTRUM_SOFT_MAX_MINOR. Proceed with caution."
    fi
  else
    echo "Supported versions are: $VECTRUM_MIN_VERSION_MAJOR.$VECTRUM_MIN_VERSION_MINOR - $VECTRUM_MAX_VERSION_MAJOR.$VECTRUM_MAX_VERSION_MINOR"
    echo "Invalid VECTRUM installation. Exiting..."
    exit 1;
  fi
}
