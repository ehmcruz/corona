#!/usr/bin/env Rscript

library(ggplot2)
library(MASS)

mean = 4.58
stdedev = 3.24

variance = stdedev*stdedev

# mean = alpha * beta
# variance = alpha * beta ^2

# m = a*b
# a = m/b

# v = a * b^2
# v = (m/b) * b^2
# v = m*b
# b = v/m

betha = variance/mean
alpha = (mean*mean) / variance

print(alpha)
print(betha)

# print("p")
# p = (1:9)/10
# p

# print("qg")
# qg = qgamma(p, shape = alpha, scale=betha)
# qg

# print("pg")
# pg = pgamma(qg, shape = alpha, scale = betha)
# pg

pdf(file=paste0("teste-incubation.pdf"), width=11)

x = rgamma(100000, shape=alpha, scale=betha)

den <- density(x)

dat <- data.frame(x = den$x, y = den$y)

# Plot density as points

ggplot(data = dat, aes(x = x, y = y)) + 
  geom_point(size = 3) +
  theme_classic()

# ------------------------------------------------------------------

pdf(file=paste0("teste-infection-exp.pdf"), width=11)

mean = 2.09

x = rexp(100000, rate=1/mean)

den <- density(x)

dat <- data.frame(x = den$x, y = den$y)

# Plot density as points

ggplot(data = dat, aes(x = x, y = y)) + 
  geom_point(size = 3) +
  xlim(0, 15)+
  theme_classic()
