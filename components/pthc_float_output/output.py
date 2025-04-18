
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID

pthc_float_output_ns = cg.esphome_ns.namespace('pthc_float_output')
PTHCFloatOutput = pthc_float_output_ns.class_('PTHCFloatOutput', output.FloatOutput, cg.Component)

CONF_WITH_TRIAC = "with_triac"
CONF_PHASE_IS_CCW = "phase_is_ccw"
CONF_MAX_PWR = "max_pwr"
CONF_REL_ON_DELAY_US = "rel_on_delay_us"
CONF_REL_OFF_DELAY_US = "rel_off_delay_us"

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(PTHCFloatOutput),
    cv.Optional(CONF_WITH_TRIAC, default=False): cv.boolean,
    cv.Optional(CONF_PHASE_IS_CCW, default=False): cv.boolean,
    cv.Optional(CONF_MAX_PWR, default=3000): cv.int_range(min=1, max=15000),
    cv.Optional(CONF_REL_ON_DELAY_US, default=[0]*8): cv.All([cv.int_], cv.Length(min=8, max=8)),
    cv.Optional(CONF_REL_OFF_DELAY_US, default=[0]*8): cv.All([cv.int_], cv.Length(min=8, max=8)),
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield output.register_output(var, config)

    cg.add(var.set_with_triac(config[CONF_WITH_TRIAC]))
    cg.add(var.set_phase_is_ccw(config[CONF_PHASE_IS_CCW]))
    cg.add(var.set_max_pwr(config[CONF_MAX_PWR]))
    cg.add(var.set_rel_on_delay_us(config[CONF_REL_ON_DELAY_US]))
    cg.add(var.set_rel_off_delay_us(config[CONF_REL_OFF_DELAY_US]))
