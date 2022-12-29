RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

if [[ $# -eq 0 ]] ; then
    echo 'usage: test.sh file'
    echo '   file: tinyC compiler executable'
    exit 0
fi

echo "Testing..."
mkdir -p out
mkdir -p diff
SUCCESS=0
FAILED=0

unset PIDS
i=0

# First, run all the test concurrently
for file in *.tc; do
  $1 -r -ODC -OS -ODF "${file}" > "out/${file%%.*}.out" 2> "/dev/null" &
  PIDS[${i}]=$!
  i=$(($i + 1))
done
# Wait for all of them to complete
wait ${PIDS[@]}

# And compare the results with reference
for file in *.tc; do
  diff "expected/${file%%.*}.ref" "out/${file%%.*}.out" > "diff/${file%%.*}.diff"
  if [ $? -ne 0 ]; then
    let FAILED=FAILED+1
    printf "${RED}${file} failed${NC}\n"
  else
    printf "${GREEN}${file} succeeded${NC}\n"
    let SUCCESS=SUCCESS+1
  fi
done

echo "Failed: ${FAILED}"
echo "Succeeded: ${SUCCESS}"
