../tools/profiling/test/test.py -s suite_config.yaml -t test_config.yaml -n node_config.yaml 2>&1 | tee tmp.log
grep  "Result of Test" tmp.log | cut -d":" -f2- >> last.log
echo "This is a test report automatically generated by the INGA hardware test suite" > report.log
cd ../tools/inga/inga_tool/
make
cd -
../tools/inga/inga_tool/inga_tool  -s -d /dev/ttyUSB1 | grep "Serial:" >> report.log
../tools/inga/inga_tool/inga_tool  -s -d /dev/ttyUSB1 | grep "Serial:" | cut -f 2 -d" " > tapeprint
#lpr -o landscape -o cpi=9 -o lpi=4 -o position=top-right -PTape tapeprint
rm tapeprint
sed -r "s/\x1B\[([0-9]{0,2}(;[0-9]{1,2})?)?[m|K]//g" last.log >> report.log
rm tmp.log
rm last.log
#lpr -Popo report.log
