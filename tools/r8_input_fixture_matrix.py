#!/usr/bin/env python3
"""Replay versioned input recipes against the actual shared native input owners."""
import argparse
import json
import math
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ACTIONS={name:i for i,name in enumerate(('Up','Down','Left','Right','Confirm','Cancel','Menu','Map','Quest','QuickItem'))}
SOURCES={'keyboard':0,'gamepad':1,'touch':2,'enhanced':3}
PHASES={'Pressed':0,'Released':1,'Canceled':2,'Moved':3}
FIELDS=('saveUsable','mapReady','uiModal','dialoguePending','scriptBusy','battleBusy',
        'uiAttached','fieldAttached','battleAttached','ioBusy')
PIN='70db90c9077aed1272e746fc2537d9f12b95a91c'


def encode(step):
    op=step['op']
    epoch=step.get('epoch',2)
    if type(epoch) is not int or not 0<=epoch<2**64:raise ValueError('invalid epoch')
    if op=='context':
        context=step.get('context',{})
        if not set(context)<=set(FIELDS)|{'suspensionReasons','hostGeneration'}:raise ValueError('unknown context fact')
        values=[context.get(field,False) for field in FIELDS]
        if any(type(v) is not bool for v in values):raise ValueError('context flags must be boolean')
        reasons=context.get('suspensionReasons',0);generation=context.get('hostGeneration',0)
        if type(reasons) is not int or not 0<=reasons<=255:raise ValueError('invalid suspension mask')
        if type(generation) is not int or not 0<=generation<2**64:raise ValueError('invalid generation')
        return 'context '+' '.join(str(int(v)) for v in values)+f' {reasons} {generation}'
    if op in ('event','physical'):
        if step.get('source') not in SOURCES:raise ValueError('unknown source')
        if step.get('action') not in ACTIONS:raise ValueError('unknown action')
        if step.get('phase') not in ('Pressed','Released','Canceled'):raise ValueError('unknown event phase')
        device=step.get('device',0);control=step.get('control',0)
        if any(type(v) is not int or not 0<=v<=65535 for v in (device,control)):raise ValueError('invalid control identity')
        return f"{op} {SOURCES[step['source']]} {device} {control} {ACTIONS[step['action']]} {PHASES[step['phase']]} {epoch}"
    if op=='touch':
        phase=step['phase'];finger=step['finger'];x=step.get('x',0);y=step.get('y',0)
        if phase not in PHASES or type(finger) is not int or not 0<=finger<=65535:raise ValueError('invalid touch capture')
        if any(type(v) not in (int,float) or not math.isfinite(v) for v in (x,y)):raise ValueError('invalid touch coordinate')
        return f'touch {PHASES[phase]} {finger} {x} {y} {epoch}'
    if op=='axis':
        if step.get('source') not in ('gamepad','enhanced'):raise ValueError('invalid axis source')
        x=step['x'];y=step['y']
        if any(type(v) not in (int,float) or not math.isfinite(v) for v in (x,y)):raise ValueError('invalid axis coordinate')
        return f"axis {SOURCES[step['source']]} {x} {y} {epoch}"
    if op=='suspend':
        if step['reason'] not in ('background','inactive','paused') or type(step['active']) is not bool:raise ValueError('invalid reason')
        return f"suspend {dict(background=1,inactive=2,paused=4)[step['reason']]} {int(step['active'])}"
    if op=='fence':return 'fence'
    if op=='tick':
        count=step['count'];enabled=step['enabled']
        if type(count) is not int or not 0<=count<=20000 or type(enabled) is not bool:raise ValueError('invalid tick batch')
        return f'tick {count} {int(enabled)}'
    raise ValueError('unknown operation '+str(op))


def validate(recipe):
    if type(recipe.get('version')) is not int or recipe['version']!=1 or recipe.get('source_pin')!=PIN:
        raise ValueError('unsupported version/provenance')
    seen=set()
    cases=recipe.get('cases')
    if not isinstance(cases,list) or not cases:raise ValueError('empty fixture matrix')
    for case in cases:
        identity=case['id']
        if identity in seen:raise ValueError('duplicate case '+identity)
        seen.add(identity)
        if not case.get('steps'):raise ValueError('empty case '+identity)
        for step in case['steps']:
            encode(step)
            if not isinstance(step.get('expect'),dict) or not step['expect']:raise ValueError('missing expectation')
            if not set(step['expect'])<= {'status','target','focus','epoch','held','repeat_up','repeat_down'}:
                raise ValueError('unknown expectation field')


def replay(probe,recipe):
    validate(recipe)
    observations=0
    for case in recipe['cases']:
        commands='\n'.join(encode(step) for step in case['steps'])+'\n'
        run=subprocess.run([str(probe)],input=commands,capture_output=True,text=True,check=True,timeout=30)
        actual=[json.loads(line) for line in run.stdout.splitlines()]
        if len(actual)!=len(case['steps']):raise AssertionError(case['id']+' incomplete observations')
        for index,(step,observed) in enumerate(zip(case['steps'],actual)):
            for key,value in step['expect'].items():
                if observed.get(key)!=value:
                    raise AssertionError(f"{case['id']} step {index}: {key}: expected {value!r}, observed {observed.get(key)!r}")
            observations+=1
    return {'cases':len(recipe['cases']),'observations':observations,'runtime_certified':False}


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--probe',required=True,type=Path)
    parser.add_argument('--fixtures',type=Path,default=ROOT/'tests/fixtures/r8/input_matrix.json')
    args=parser.parse_args()
    result=replay(args.probe,json.loads(args.fixtures.read_text()))
    print(json.dumps(result,sort_keys=True))


if __name__=='__main__':main()
