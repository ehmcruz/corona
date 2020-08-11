#!/usr/bin/env Rscript

args = commandArgs(trailingOnly=TRUE)

if (length(args) < 1)
	stop("Usage: plot-cycles.R <csv>\n")

prefix = args[1]

options(scipen=999)


data = read.csv(prefix, header = TRUE)

total_pop = data[1, "ac_state_ST_HEALTHY"]
          + data[1, "ac_state_ST_INFECTED"]
          + data[1, "ac_state_ST_IMMUNE"]
          + data[1, "ac_state_ST_DEAD"]

print(total_pop)

data$total_infected = data$ac_state_ST_INFECTED + data$ac_state_ST_IMMUNE + data$ac_state_ST_DEAD

col_reported <- numeric(nrow(data))

before = 0
for (i in 1:length(col_reported)) {
	col_reported[i] = before + data[i, "infected_state_ST_MILD"] # a person always become MILD before SEVERE and CRITICAL
	before = col_reported[i]
}

data$total_reported = col_reported

data$reported = data$ac_infected_state_ST_MILD + data$ac_infected_state_ST_SEVERE + data$ac_infected_state_ST_CRITICAL

# -----------------------------------------------------

#pdf(file=paste0(prefix, "-cycles.pdf"), width=11)
png(filename = paste0(prefix, "-cycles.png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

#labels = c("sim-infected", "sim-suscetive", "sir-infected", "sir-suscetive")
labels = c("sim-infected", "sim-suscetive")

plot(data$cycle, data$ac_state_ST_HEALTHY, ylim=c(0,total_pop), type="o", col="green", xlab="Dias desde paciente zero", ylab="Total de pessoas")

lines(data$cycle, data$ac_state_ST_INFECTED, type="o", col="orange")

#lines(data$cycle, data$sir_i, ylim=c(0,myylim), type="o", col="gray")
#lines(data$cycle, data$sir_s, ylim=c(0,myylim), type="o", col="gray")

grid(col = "gray", lwd=2)

#legend(150, 100000, labels, cex=0.8, col=c("orange","green","gray","gray"), pch=21:22, lty=1:2);
legend(150, 50000, labels, cex=0.8, col=c("orange", "green"), pch=21:22, lty=1:2);

# -----------------------------------------------------

#pdf(file=paste0(prefix, "-critical.pdf"), width=11)
png(filename = paste0(prefix, "-critical.png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

labels = c("sim-critical")

plot(data$cycle, data$ac_infected_state_ST_CRITICAL, xlab="Dias desde paciente zero", ylab="Casos críticos - necessitam de UTI", type="o", col="red")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("red"), pch=21:22, lty=1:2);

# -----------------------------------------------------

#pdf(file=paste0(prefix, "-severe.pdf"), width=11)
png(filename = paste0(prefix, "-severe.png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

labels = c("sim-severe")

plot(data$cycle, data$ac_infected_state_ST_SEVERE, xlab="Dias desde paciente zero", ylab="Casos severos - necessitam de Internação na Enfermaria", type="o", col="orange")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("orange"), pch=21:22, lty=1:2);

# -----------------------------------------------------

#pdf(file=paste0(prefix, "-mild.pdf"), width=11)
png(filename = paste0(prefix, "-mild.png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

labels = c("sim-mild")

plot(data$cycle, data$ac_infected_state_ST_MILD, xlab="Dias desde paciente zero", ylab="Casos moderados - podem se recuperar em casa", type="o", col="pink")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("pink"), pch=21:22, lty=1:2);

# -----------------------------------------------------

#pdf(file=paste0(prefix, "-mild.pdf"), width=11)
png(filename = paste0(prefix, "-total-infected.png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

labels = c("sim-infected")

plot(data$cycle, data$total_infected, xlab="Dias desde paciente zero", ylab="Total acumulado de infectados", type="o", col="pink")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("purple"), pch=21:22, lty=1:2);

# -----------------------------------------------------

#pdf(file=paste0(prefix, "-mild.pdf"), width=11)
png(filename = paste0(prefix, "-total-reported.png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

labels = c("sim-infected")

plot(data$cycle, data$total_reported, xlab="Dias desde paciente zero", ylab="Total acumulado de infectados reportados", type="o", col="pink")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("purple"), pch=21:22, lty=1:2);

# -----------------------------------------------------

#pdf(file=paste0(prefix, "-mild.pdf"), width=11)
png(filename = paste0(prefix, "-reported.png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

labels = c("sim-infected")

plot(data$cycle, data$reported, xlab="Dias desde paciente zero", ylab="Total acumulado de infectados reportados", type="o", col="pink")

grid(col = "gray", lwd=2)

legend(150, 10000, labels, cex=0.8, col=c("purple"), pch=21:22, lty=1:2);
