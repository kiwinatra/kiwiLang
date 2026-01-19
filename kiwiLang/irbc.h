class _IRB {
public:
    _LV* _create_xr_obj_new(_LT* _ct, 
                           const std::string& _cn,
                           _LV* _out = nullptr,
                           const std::string& _nm = "");
    
    _LV* _create_xr_obj_del(_LV* _obj);
    
    _LV* _create_xr_prop_get(_LV* _obj,
                            const std::string& _pn,
                            _LT* _pt);
    
    _LV* _create_xr_prop_set(_LV* _obj,
                            const std::string& _pn,
                            _LV* _val);
    
    _LV* _create_xr_func_call(_LV* _obj,
                             const std::string& _fn,
                             _LV* _args,
                             _LT* _rt = nullptr);
    
    _LV* _create_xr_dlg_bind(_LV* _dlg,
                            _LV* _obj,
                            const std::string& _fn);
    
    _LV* _create_xr_dlg_exec(_LV* _dlg,
                            _LV* _args);
    
    _LV* _create_xr_cls_def(const std::string& _cn);
    _LV* _create_xr_cls_static(const std::string& _cn);
    
    _LV* _create_bp_node(_LV* _gph,
                        const std::string& _nt,
                        _LV* _pins);
    
    _LV* _create_bp_pin_get(_LV* _nd,
                           const std::string& _pn,
                           _LT* _pt);
    
    _LV* _create_bp_pin_set(_LV* _nd,
                           const std::string& _pn,
                           _LV* _val);
    
    _LV* _create_xr_malloc(_LT* _t, _LV* _sz);
    _LV* _create_xr_free(_LV* _ptr);
    
    _LV* _create_xr_add_root(_LV* _obj);
    _LV* _create_xr_rem_root(_LV* _obj);
    
private:
    _LF* _get_xr_rt_func(const std::string& _nm,
                        _LFT* _t);
};