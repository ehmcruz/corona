#!/usr/bin/env Rscript

args = commandArgs(trailingOnly=TRUE)

if (length(args) < 1)
	stop("Usage: plot-cycles.R <csv>\n")

prefix = args[1]

options(scipen=999)


data = read.csv(prefix, header = TRUE)

# -----------------------------------------------------

pdf(file=paste0(prefix, "-cycles.pdf"), width=11)

myylim = 100000

#labels = c("sim-infected", "sim-suscetive", "sir-infected", "sir-suscetive")
labels = c("sim-infected", "sim-suscetive")

plot(data$cycle, data$ac_state_ST_INFECTED, ylim=c(0,myylim), xlab="Dias desde paciente zero", ylab="Total de pessoas", type="o", col="orange")

lines(data$cycle, data$ac_state_ST_HEALTHY, ylim=c(0,myylim), type="o", col="green")

#lines(data$cycle, data$sir_i, ylim=c(0,myylim), type="o", col="gray")
#lines(data$cycle, data$sir_s, ylim=c(0,myylim), type="o", col="gray")

grid(col = "gray", lwd=2)

#legend(150, 100000, labels, cex=0.8, col=c("orange","green","gray","gray"), pch=21:22, lty=1:2);
legend(150, 50000, labels, cex=0.8, col=c("orange", "green"), pch=21:22, lty=1:2);

# -----------------------------------------------------

pdf(file=paste0(prefix, "-critical.pdf"), width=11)

labels = c("sim-critical")

plot(data$cycle, data$ac_infected_state_ST_CRITICAL, xlab="Dias desde paciente zero", ylab="Casos críticos - necessitam de UTI", type="o", col="red")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("red"), pch=21:22, lty=1:2);

# -----------------------------------------------------

pdf(file=paste0(prefix, "-severe.pdf"), width=11)

labels = c("sim-severe")

plot(data$cycle, data$ac_infected_state_ST_SEVERE, xlab="Dias desde paciente zero", ylab="Casos severos - necessitam de Internação na Enfermaria", type="o", col="orange")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("orange"), pch=21:22, lty=1:2);

# -----------------------------------------------------

pdf(file=paste0(prefix, "-mild.pdf"), width=11)

labels = c("sim-mild")

plot(data$cycle, data$ac_infected_state_ST_MILD, xlab="Dias desde paciente zero", ylab="Casos moderados - podem se recuperar em casa", type="o", col="pink")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("pink"), pch=21:22, lty=1:2);
