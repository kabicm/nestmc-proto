#pragma once

#include <cell_group.hpp>
#include <pss_cell_description.hpp>
#include <random>
#include <vector>

namespace nest {
namespace mc {

// Cell group where each cell produces Poisson distributed spikes independently
// and possibly with different means. These cells serve only to produce spikes and
// should not receive any events! In case of incoming events, the exception is thrown.
class pss_cell_group: public cell_group {
public:

    using value_type = double;

    pss_cell_group() = default;

    // Constructor containing gid of first cell in a group and a container of all cells.
    pss_cell_group(cell_gid_type first_gid, const std::vector<util::unique_any>& cells):
        gid_base_{first_gid}, exp_dist_(1.0)
    {
        cells_.reserve(cells.size());
        // Cast each cell to pss_cell_description.
        for(const auto& cell : cells) {
            cells_.push_back(util::any_cast<pss_cell_description>(cell));
        }

        // Hardcoded seed!
        // TODO: Make general architecture for random seeds for the whole library.
        generator_.seed(3521 + first_gid_);

        // Sample times of next spike for each cell.
        // This is necessary since the method "advance" assumes
        // that the spike times are sampled in the previous step.
        next_spike_time_.reserve(cells.size());
        for (auto i: util::make_span(0, cells_.size())) {
            // Since lambda=1/rate, this corresponds to Poisson(rate) distribution
            next_spike_time_[i] = exp_dist_(generator_) * cells_[i].lambda;
        }
    }

    cell_kind get_cell_kind() const override {
        return cell_kind::poisson_spike_source;
    }

    // Produces Poisson-distributed spikes up to tfinal.
    // Parameter dt is ignored!
    void advance(time_type tfinal, time_type dt) override {
        // Iterates over cells.
        for (auto i: util::make_span(0, cells_.size())) {
            while(next_spike_time_[i] < tfinal) {
                // Produce spike from the previously sample spike time
                cell_member_type spike_neuron_gid = {gid_base_ + cell_gid_type(i), 0};
                spike s = {spike_neuron_gid, next_spike_time_[i]};
                spikes_.push_back(s);

                // Sample next spike time of this cell.
                next_spike_time_[i] += exp_dist_(generator_) * cells_[i].lambda;
            }
        }
    }

    // Poisson cell serves only to produce spikes and should not receive any events.
    // Throw the exception, since this might indicate a bug in the recipe.
    void enqueue_events(const std::vector<postsynaptic_spike_event>& events) override {
        std::logic_error("The pss_cells do not support incoming events!");
    }

    const std::vector<spike>& spikes() const override {
        return spikes_;
    }

    void clear_spikes() override {
        spikes_.clear();
    }

    void add_sampler(cell_member_type probe_id, sampler_function s, time_type start_time = 0) override {
        std::logic_error("The pss_cells do not support sampling of internal state!");
    }

    void set_binning_policy(binning_kind policy, time_type bin_interval) override {
    }

    // Poisson cells have no probes.
    std::vector<probe_record> probes() const override {
        return std::vector<probe_record>();
    }

    void reset() override {
        spikes_.clear();
        next_spike_time_.clear();
    }

private:
    // Gid of first cell in group.
    cell_gid_type gid_base_;

    // Cells that belong to this group.
    std::vector<pss_cell_description> cells_;

    // Spikes that are generated (not necessarily sorted).
    std::vector<spike> spikes_;

    // Random number generator.
    std::minstd_rand generator_;

    // Unit exponential distribution (with mean 1).
    std::exponential_distribution<time_type> exp_dist_;

    // Sampled next spike time of each cell.
    std::vector<time_type> next_spike_time_;
};
} // namespace mc
} // namespace nest