#!/usr/bin/env python3

import numpy

count = 1024
row = 16
scale = 64

# generate 1000 values between 0 and 1
#data = numpy.random.rand(count)
# shift values in the 0 .. 64 interval
#data = data * scale

data = numpy.random.exponential(scale, count)

# convert to integer values
data = data.astype(int)


print("// clang-format off")
print("const std::array<unsigned, {}> data =".format(count))
print("   {")

for ii in range(count // row):
    pp = data[(ii*row):(ii+1)*row].astype(str).tolist()
    print("    {},".format(", ".join(pp)))

print("   };")

quartiles = numpy.percentile(data, [25, 50, 75])
data_min, data_max = data.min(), data.max()

print("const ftags::FiveNumberSummary<unsigned> expected{")
# print 5-number summary
print("/* minimum       = */ {},".format(data_min))
print("/* lowerQuartile = */ {},".format(int(quartiles[0])))
print("/* median        = */ {},".format(int(quartiles[1])))
print("/* upperQuartile = */ {},".format(int(quartiles[2])))
print("/* maximum       = */ {}".format(data_max))
print("};")

print("// clang-format on")
