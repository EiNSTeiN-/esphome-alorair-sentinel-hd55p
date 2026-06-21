from esphome import automation
import esphome.codegen as cg
from esphome.components import climate
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TURN_OFF_ACTION, CONF_TURN_ON_ACTION

CONF_TARGET_HUMIDITY_CHANGE_ACTION = "target_humidity_change_action"

AUTO_LOAD = ["climate"]
CODEOWNERS = ["@EiNSTeiN-"]

alorair_hd55p_ns = cg.esphome_ns.namespace("alorair_hd55p")
AlorairHD55PClimate = alorair_hd55p_ns.class_(
    "AlorairHD55PClimate", climate.Climate, cg.Component
)

CONFIG_SCHEMA = (
    climate.climate_schema(AlorairHD55PClimate)
    .extend(
        {
            cv.Optional(CONF_TURN_ON_ACTION): automation.validate_automation(
                single=True
            ),
            cv.Optional(CONF_TURN_OFF_ACTION): automation.validate_automation(
                single=True
            ),
            cv.Optional(
                CONF_TARGET_HUMIDITY_CHANGE_ACTION
            ): automation.validate_automation(single=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    if turn_on_action := config.get(CONF_TURN_ON_ACTION):
        await automation.build_automation(
            var.get_turn_on_trigger(), [], turn_on_action
        )

    if turn_off_action := config.get(CONF_TURN_OFF_ACTION):
        await automation.build_automation(
            var.get_turn_off_trigger(), [], turn_off_action
        )

    if target_humidity_action := config.get(CONF_TARGET_HUMIDITY_CHANGE_ACTION):
        await automation.build_automation(
            var.get_target_humidity_change_trigger(),
            [(cg.float_, "x")],
            target_humidity_action,
        )
