const minimumSamplingTime = 500
const minimumSamplingCount = 30

const maximumSamplingTime = 60000
const maximumRelativeError = 0.01

const Z = 1.96

console.log('TAP version 13')

let n = 0

globalThis.bench = function bench(name, fn) {
  const start = now()

  console.log(`\n# ${name}`)

  let operations = 0
  let elapsed = 0
  let iterations = 1

  /**
   * Step 1: Estimate the period of the function by sampling for at least the
   * minimum sampling time and taking at least the minimum sampling count.
   */

  while (elapsed < minimumSamplingTime || operations < minimumSamplingCount) {
    const start = now()

    for (let i = 0; i < iterations; i++) fn()

    const sample = now() - start

    operations += iterations
    elapsed += sample
    iterations *= 2
  }

  /**
   * Step 2: Collect samples for up to the maximum sampling time or until the
   * confidence interval is within the desired precision relative to the mean.
   */

  const samples = []

  while (elapsed < maximumSamplingTime) {
    const start = now()

    for (let i = 0; i < iterations; i++) fn()

    const sample = now() - start

    const period = sample / iterations

    samples.push((1 / period) * 1e3)

    operations += iterations
    elapsed += sample

    if (samples.length > 2) {
      const m = mean(samples)

      if (confidence(samples, Z, m) / m < maximumRelativeError) break
    }
  }

  console.log('    #', mean(samples) | 0, 'ops/s')
  console.log(`ok ${++n} - ${name} # time = ${(now() - start) | 0}ms`)
}

let now

if (typeof process === 'object') {
  now = function now() {
    const time = process.hrtime()
    return time[0] * 1e3 + time[1] / 1e6
  }
} else if (typeof performance === 'object') {
  now = function now() {
    return performance.now()
  }
} else {
  now = function now() {
    return Date.now()
  }
}

function sum(samples) {
  return samples.reduce((a, b) => a + b, 0)
}

function mean(samples) {
  return sum(samples) / samples.length
}

function variance(samples, m = mean(samples)) {
  return mean(samples.map((n) => (n - m) ** 2))
}

function deviation(samples, m = mean(samples), v = variance(samples, m)) {
  return Math.sqrt(v)
}

function error(
  samples,
  m = mean(samples),
  v = variance(samples, m),
  d = deviation(samples, m, v)
) {
  return d / Math.sqrt(samples.length)
}

function confidence(
  samples,
  Z,
  m = mean(samples),
  v = variance(samples, m),
  d = deviation(samples, m, v),
  e = error(samples, m, v, d)
) {
  return Z * e * 2
}
